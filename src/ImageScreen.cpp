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

  // Build the request URL to the dithering server using actual display dimensions (swapped for rotation)
  String requestUrl = String(imageServerUrl) + "/process?url=" + urlEncode(String(imageUrl)) +
                      "&width=" + String(display.height()) + "&height=" + String(display.width()) + "&dither=true";

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

  // Get the payload as a string first to handle chunked encoding properly
  String payload = http.getString();

  if (payload.length() == 0) {
    Serial.println("Empty payload received");
    http.end();
    return false;
  }

  Serial.printf("Received payload of %d bytes\n", payload.length());

  // Convert string to byte array for BMP processing
  const uint8_t* data = (const uint8_t*)payload.c_str();
  size_t dataSize = payload.length();
  size_t dataIndex = 0;

  // Read BMP header
  if (dataSize < 54) {
    Serial.printf("Payload too small for BMP header: got %d bytes, expected at least 54\n", dataSize);
    http.end();
    return false;
  }

  uint8_t bmpHeader[54];
  memcpy(bmpHeader, data + dataIndex, 54);
  dataIndex += 54;
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
  if (dataIndex + 16 > dataSize) {
    Serial.printf("Not enough data for color palette: need %d bytes, have %d\n", dataIndex + 16, dataSize);
    http.end();
    return false;
  }

  uint8_t palette[16];
  memcpy(palette, data + dataIndex, 16);
  dataIndex += 16;
  Serial.println("Color palette read successfully");

  // Skip any remaining header bytes until data offset
  if (dataOffset > dataIndex) {
    uint32_t skipBytes = dataOffset - dataIndex;
    if (dataIndex + skipBytes > dataSize) {
      Serial.printf("Data offset beyond payload size: offset %d, payload size %d\n", dataOffset, dataSize);
      http.end();
      return false;
    }
    dataIndex += skipBytes;
  }

  // Initialize display
  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);

  // Render the image over the whole screen (no offset)
  int offsetX = 0;
  int offsetY = 0;

  Serial.printf("Display: %dx%d, Image: %dx%d, Offset: (%d,%d)\n", display.width(), display.height(), imageWidth,
                imageHeight, offsetX, offsetY);

  // Read and display the image data
  // Palette was already read above

  // Process image data (BMP is stored bottom-to-top)
  uint32_t rowSize = ((imageWidth * bitsPerPixel + 31) / 32) * 4;  // Row size padded to 4-byte boundary
  uint8_t* rowBuffer = new uint8_t[rowSize];

  for (int y = imageHeight - 1; y >= 0; y--) {
    if (dataIndex + rowSize > dataSize) {
      Serial.printf("Not enough data for image row: need %d bytes, have %d remaining\n", rowSize, dataSize - dataIndex);
      delete[] rowBuffer;
      http.end();
      return false;
    }

    memcpy(rowBuffer, data + dataIndex, rowSize);
    dataIndex += rowSize;

    // Calculate the correct display Y coordinate (BMP is bottom-to-top, so we need to flip it)
    int displayRowY = offsetY + y;

    // Debug: show progress every 20 rows
    if (y % 20 == 0) {
      Serial.printf("Reading BMP row %d (will display at y=%d)\n", y, displayRowY);
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

      int displayX = offsetX + x;
      int displayY = displayRowY;

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

  display.displayWindow(0, 0, display.width(), display.height());
  display.hibernate();

  Serial.println("Image display complete");
}