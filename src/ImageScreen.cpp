#include "ImageScreen.h"

#include <WiFi.h>

#include "battery.h"

ImageScreen::ImageScreen(DisplayType& display, ApplicationConfig& config)
    : display(display),
      config(config),
      smallFont(u8g2_font_helvR08_tr),
      ditheringServiceUrl("https://dither.shvn.dev") {
  gfx.begin(display);
}

void ImageScreen::storeImageETag(const String& etag) {
  strncpy(storedImageETag, etag.c_str(), sizeof(storedImageETag) - 1);
  storedImageETag[sizeof(storedImageETag) - 1] = '\0';
  Serial.println("Stored ETag: " + etag);
}

String ImageScreen::getStoredImageETag() { return String(storedImageETag); }

int ImageScreen::downloadAndDisplayImage() {
  HTTPClient http;

  String requestUrl = ditheringServiceUrl + "/process?url=" + urlEncode(String(config.imageUrl)) +
                      "&width=" + String(display.height()) + "&height=" + String(display.width()) + "&dither=true";

  Serial.println("Requesting image from: " + requestUrl);

  http.begin(requestUrl);
  http.setTimeout(10000);

  String storedETag = getStoredImageETag();
  if (storedETag.length() > 0) {
    http.addHeader("If-None-Match", storedETag);
  }

  const char* headerKeys[] = {"Content-Type", "Transfer-Encoding", "ETag"};
  size_t headerKeysSize = sizeof(headerKeys) / sizeof(char*);
  http.collectHeaders(headerKeys, headerKeysSize);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_NOT_MODIFIED) {
    Serial.println("Image not modified (304), using cached version");
    http.end();
    return httpCode;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP request failed with code: %d\n", httpCode);
    Serial.printf("HTTP error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return httpCode;
  }

  // Store new ETag if present
  String newETag = http.header("ETag");
  if (newETag.length() > 0) {
    storeImageETag(newETag);
  }

  String contentType = http.header("Content-Type");
  contentType.toLowerCase();
  if (!contentType.isEmpty() && contentType != "image/bmp") {
    Serial.println("Unexpected content type: " + contentType);
    http.end();
    return -1;
  }

  String payload = http.getString();

  if (payload.length() == 0) {
    Serial.println("Empty payload received");
    http.end();
    return -1;
  }

  const uint8_t* data = (const uint8_t*)payload.c_str();
  size_t dataSize = payload.length();
  size_t dataIndex = 0;

  if (dataSize < 54) {
    Serial.printf("Payload too small for BMP header: got %d bytes, expected at least 54\n", dataSize);
    http.end();
    return -1;
  }

  uint8_t bmpHeader[54];
  memcpy(bmpHeader, data + dataIndex, 54);
  dataIndex += 54;

  if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
    Serial.printf("Invalid BMP signature: got 0x%02X 0x%02X, expected 0x42 0x4D ('BM')\n", bmpHeader[0], bmpHeader[1]);
    http.end();
    return -1;
  }

  uint32_t dataOffset = bmpHeader[10] | (bmpHeader[11] << 8) | (bmpHeader[12] << 16) | (bmpHeader[13] << 24);
  uint32_t imageWidth = bmpHeader[18] | (bmpHeader[19] << 8) | (bmpHeader[20] << 16) | (bmpHeader[21] << 24);
  uint32_t imageHeight = bmpHeader[22] | (bmpHeader[23] << 8) | (bmpHeader[24] << 16) | (bmpHeader[25] << 24);
  uint16_t bitsPerPixel = bmpHeader[28] | (bmpHeader[29] << 8);
  uint32_t compression = bmpHeader[30] | (bmpHeader[31] << 8) | (bmpHeader[32] << 16) | (bmpHeader[33] << 24);

  if (bitsPerPixel != 8) {
    Serial.printf("Unsupported bits per pixel: %d (expected 8 for indexed color)\n", bitsPerPixel);
    http.end();
    return -1;
  }

  if (compression != 0) {
    Serial.printf("Unsupported compression: %d (expected 0 for uncompressed)\n", compression);
    http.end();
    return -1;
  }

  if (dataIndex + 16 > dataSize) {
    Serial.printf("Not enough data for color palette: need %d bytes, have %d\n", dataIndex + 16, dataSize);
    http.end();
    return -1;
  }

  uint8_t palette[16];
  memcpy(palette, data + dataIndex, 16);
  dataIndex += 16;

  if (dataOffset > dataIndex) {
    uint32_t skipBytes = dataOffset - dataIndex;
    if (dataIndex + skipBytes > dataSize) {
      Serial.printf("Data offset beyond payload size: offset %d, payload size %d\n", dataOffset, dataSize);
      http.end();
      return -1;
    }
    dataIndex += skipBytes;
  }

  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);

  int offsetX = 0;
  int offsetY = 0;

  // Row size padded to 4-byte boundary
  uint32_t rowSize = ((imageWidth * bitsPerPixel + 31) / 32) * 4;
  uint8_t* rowBuffer = new uint8_t[rowSize];

  // Create buffer for 2-bit grey pixmap (4 pixels per byte)
  int16_t pixmapWidth = (imageWidth + 3) / 4;  // bytes per row for 2-bit depth
  size_t pixmapSize = pixmapWidth * imageHeight;
  uint8_t* greyPixmap = new uint8_t[pixmapSize];
  memset(greyPixmap, 0, pixmapSize);

  for (int y = imageHeight - 1; y >= 0; y--) {
    if (dataIndex + rowSize > dataSize) {
      Serial.printf("Not enough data for image row: need %d bytes, have %d remaining\n", rowSize, dataSize - dataIndex);
      delete[] rowBuffer;
      delete[] greyPixmap;
      http.end();
      return -1;
    }

    memcpy(rowBuffer, data + dataIndex, rowSize);
    dataIndex += rowSize;

    int pixmapRowY = y;

    for (int x = 0; x < imageWidth; x++) {
      uint8_t pixelIndex = rowBuffer[x];

      // Pack 4 pixels per byte (2 bits each)
      int byteIndex = pixmapRowY * pixmapWidth + x / 4;
      int bitShift = 6 - (x % 4) * 2;  // 6, 4, 2, 0 for positions 0, 1, 2, 3

      greyPixmap[byteIndex] |= (pixelIndex << bitShift);
    }
  }

  delete[] rowBuffer;

  display.drawGreyPixmap(greyPixmap, 2, offsetX, offsetY, imageWidth, imageHeight);

  delete[] greyPixmap;
  http.end();

  return HTTP_CODE_OK;
}

void ImageScreen::render() {
  int statusCode = downloadAndDisplayImage();

  if (statusCode == HTTP_CODE_NOT_MODIFIED) {
    return;
  }

  if (statusCode == HTTP_CODE_OK) {
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    return;
  }

  String errorMessage;
  switch (statusCode) {
    case -1:
      errorMessage = "Invalid image data";
      break;
    case HTTP_CODE_NOT_FOUND:
      errorMessage = "Image not found";
      break;
    case HTTP_CODE_UNSUPPORTED_MEDIA_TYPE:
      errorMessage = "Unsupported image format";
      break;
    case HTTP_CODE_NO_CONTENT:
      errorMessage = "Empty image response";
      break;
    case HTTP_CODE_REQUEST_TIMEOUT:
      errorMessage = "Request timeout";
      break;
    case HTTP_CODE_SERVICE_UNAVAILABLE:
      errorMessage = "Service unavailable";
      break;
    default:
      errorMessage = "Failed to load image";
      break;
  }

  displayError(errorMessage);
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

int ImageScreen::nextRefreshInSeconds() { return 900; }
