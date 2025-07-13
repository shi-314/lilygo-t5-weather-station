#include "ImageScreen.h"

#include <WiFi.h>

#include "battery.h"

// Image URL configuration
static const char* imageUrl =
    "https://kagi.com/proxy/"
    "i?c=lWla4SiEvVNmj85b_dW2HcBDkb-62vZXR0vAz8RZagrG_NApBQBAZJFrTl05QJmqNF0XvQX_0qzHNGs0-YvfB6_Bbwo0h_"
    "AZJJSIaIfmpN2LIrIea6feEj6Tb3oFVVv6YQFY6m3Gndv6VXSbLt3sazc2SwfVYvGAB9WagAI6nu4%3D";  // Replace with your image URL

ImageScreen::ImageScreen(DisplayType& display) : display(display), smallFont(u8g2_font_helvR08_tr) {
  gfx.begin(display);
}

String ImageScreen::urlEncode(const String& str) {
  String encoded = "";
  char c;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      encoded += '%';
      if (c < 16) encoded += '0';
      encoded += String(c, HEX);
    }
  }
  return encoded;
}

void ImageScreen::displayError(const String& errorMessage) {
  Serial.println("ImageScreen Error: " + errorMessage);

  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);

  gfx.setFontMode(1);
  gfx.setForegroundColor(GxEPD_BLACK);
  gfx.setBackgroundColor(GxEPD_WHITE);
  gfx.setFont(smallFont);

  int textWidth = gfx.getUTF8Width(errorMessage.c_str());
  int textHeight = gfx.getFontAscent() - gfx.getFontDescent();

  int x = (display.width() - textWidth) / 2;
  int y = (display.height() + textHeight) / 2;

  gfx.setCursor(x, y);
  gfx.print(errorMessage);

  display.displayWindow(0, 0, display.width(), display.height());
  display.hibernate();
}

bool ImageScreen::downloadAndDisplayImage() {
  HTTPClient http;

  // Build the request URL to the dithering server
  String requestUrl = String(imageServerUrl) + "/process?url=" + urlEncode(String(imageUrl)) +
                      "&width=" + String(displayWidth) + "&height=" + String(displayHeight) + "&dither=true";

  Serial.println("Requesting image from: " + requestUrl);

  http.begin(requestUrl);
  http.setTimeout(30000);  // 30 second timeout

  // Collect essential headers
  const char* headerKeys[] = {"Content-Type", "Transfer-Encoding"};
  size_t headerKeysSize = sizeof(headerKeys) / sizeof(char*);
  http.collectHeaders(headerKeys, headerKeysSize);

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP request failed with code: %d\n", httpCode);
    Serial.printf("HTTP error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }

  Serial.println("HTTP request successful");

  String contentType = http.header("Content-Type");
  Serial.printf("Content-Type: '%s'\n", contentType.c_str());

  contentType.toLowerCase();
  if (!contentType.isEmpty() && contentType != "image/bmp") {
    Serial.println("Unexpected content type: " + contentType);
    http.end();
    return false;
  }

  // Get the stream
  WiFiClient* stream = http.getStreamPtr();

  // Handle chunked encoding if present
  String transferEncoding = http.header("Transfer-Encoding");
  bool isChunked = (transferEncoding == "chunked");

  if (isChunked) {
    Serial.println("Handling chunked encoding");
    // Read and skip chunk size line
    String chunkSizeLine = stream->readStringUntil('\n');
    chunkSizeLine.trim();

    long chunkSize = strtol(chunkSizeLine.c_str(), NULL, 16);
    if (chunkSize <= 0) {
      Serial.println("Invalid chunk size");
      http.end();
      return false;
    }
  }

  // Read BMP header
  uint8_t bmpHeader[54];
  size_t headerBytesRead = stream->readBytes(bmpHeader, 54);

  if (headerBytesRead != 54) {
    Serial.printf("Failed to read BMP header: got %d bytes, expected 54\n", headerBytesRead);
    http.end();
    return false;
  }
  Serial.println("BMP header read successfully");

  // Parse BMP header
  if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
    Serial.printf("Invalid BMP signature: got 0x%02X 0x%02X, expected 0x42 0x4D ('BM')\n", bmpHeader[0], bmpHeader[1]);
    http.end();
    return false;
  }

  uint32_t dataOffset = bmpHeader[10] | (bmpHeader[11] << 8) | (bmpHeader[12] << 16) | (bmpHeader[13] << 24);
  uint32_t imageWidth = bmpHeader[18] | (bmpHeader[19] << 8) | (bmpHeader[20] << 16) | (bmpHeader[21] << 24);
  uint32_t imageHeight = bmpHeader[22] | (bmpHeader[23] << 8) | (bmpHeader[24] << 16) | (bmpHeader[25] << 24);
  uint16_t bitsPerPixel = bmpHeader[28] | (bmpHeader[29] << 8);
  uint32_t compression = bmpHeader[30] | (bmpHeader[31] << 8) | (bmpHeader[32] << 16) | (bmpHeader[33] << 24);

  Serial.printf("BMP Info: %dx%d, %d bits per pixel, data offset: %d, compression: %d\n", imageWidth, imageHeight,
                bitsPerPixel, dataOffset, compression);

  // Validate BMP parameters
  if (bitsPerPixel != 8) {
    Serial.printf("Unsupported bits per pixel: %d (expected 8 for indexed color)\n", bitsPerPixel);
    http.end();
    return false;
  }

  if (compression != 0) {
    Serial.printf("Unsupported compression: %d (expected 0 for uncompressed)\n", compression);
    http.end();
    return false;
  }

  // For indexed color BMP, the palette comes right after the header
  // Read color palette (4 colors * 4 bytes each = 16 bytes)
  uint8_t palette[16];
  size_t paletteBytes = stream->readBytes(palette, 16);
  if (paletteBytes != 16) {
    Serial.printf("Failed to read color palette: got %d bytes, expected 16\n", paletteBytes);
    http.end();
    return false;
  }
  Serial.println("Color palette read successfully");

  // Skip any remaining header bytes until data offset
  uint32_t totalBytesRead = 54 + 16;  // header + palette
  if (dataOffset > totalBytesRead) {
    uint32_t skipBytes = dataOffset - totalBytesRead;
    while (skipBytes > 0) {
      uint8_t dummy;
      if (stream->readBytes(&dummy, 1) != 1) {
        Serial.println("Failed to skip header bytes");
        http.end();
        return false;
      }
      skipBytes--;
    }
  }

  // Initialize display
  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);

  // Calculate centering position
  int centerX = (display.width() - imageWidth) / 2;
  int centerY = (display.height() - imageHeight) / 2;

  // Read and display the image data
  // Palette was already read above

  // Process image data (BMP is stored bottom-to-top)
  uint32_t rowSize = ((imageWidth * bitsPerPixel + 31) / 32) * 4;  // Row size padded to 4-byte boundary
  uint8_t* rowBuffer = new uint8_t[rowSize];

  for (int y = imageHeight - 1; y >= 0; y--) {
    if (stream->readBytes(rowBuffer, rowSize) != rowSize) {
      Serial.println("Failed to read image row");
      delete[] rowBuffer;
      http.end();
      return false;
    }

    for (int x = 0; x < imageWidth; x++) {
      uint8_t pixelIndex = rowBuffer[x];

      // Map the 4-color palette to display colors
      uint16_t displayColor;
      switch (pixelIndex) {
        case 0:
          displayColor = GxEPD_BLACK;
          break;  // Black
        case 1:
          displayColor = GxEPD_DARKGREY;
          break;  // Dark Gray
        case 2:
          displayColor = GxEPD_LIGHTGREY;
          break;  // Light Gray
        case 3:
          displayColor = GxEPD_WHITE;
          break;  // White
        default:
          displayColor = GxEPD_WHITE;
          break;
      }

      int displayX = centerX + x;
      int displayY = centerY + y;

      if (displayX >= 0 && displayX < display.width() && displayY >= 0 && displayY < display.height()) {
        display.drawPixel(displayX, displayY, displayColor);
      }
    }
  }

  delete[] rowBuffer;
  http.end();

  return true;
}

void ImageScreen::render() {
  Serial.println("Displaying image screen");

  if (!downloadAndDisplayImage()) {
    displayError("Failed to load image");
    return;
  }

  // Display battery status
  String batteryStatus = getBatteryStatus();

  gfx.setFontMode(1);
  gfx.setForegroundColor(GxEPD_DARKGREY);
  gfx.setBackgroundColor(GxEPD_WHITE);
  gfx.setFont(smallFont);

  int batteryWidth = gfx.getUTF8Width(batteryStatus.c_str());
  gfx.setCursor(display.width() - batteryWidth - 2, display.height() - 1);
  gfx.print(batteryStatus);

  display.displayWindow(0, 0, display.width(), display.height());
  display.hibernate();

  Serial.println("Image display complete");
}