#include "ConfigurationScreen.h"

ConfigurationScreen::ConfigurationScreen(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display,
                                         const String& accessPointName, const String& accessPointPassword)
    : display(display),
      accessPointName(accessPointName),
      accessPointPassword(accessPointPassword)
{
}

String ConfigurationScreen::generateWiFiQRString() const {
    String wifiQRCodeString = "WIFI:T:WPA2;S:" + accessPointName + ";P:" + accessPointPassword + ";H:false;;";
    
    Serial.print("Generated WiFi QR String length: ");
    Serial.println(wifiQRCodeString.length());
    Serial.print("Full WiFi QR String: ");
    Serial.println(wifiQRCodeString);
    
    return wifiQRCodeString;
}

void ConfigurationScreen::drawQRCode(const String& wifiString, int x, int y, int scale) {
    const uint8_t qrCodeVersion4 = 4;
    uint8_t qrCodeDataBuffer[qrcode_getBufferSize(qrCodeVersion4)];
    QRCode qrCodeInstance;
    
    Serial.print("Attempting to generate QR code for: ");
    Serial.println(wifiString);
    Serial.print("String length: ");
    Serial.println(wifiString.length());
    
    int qrGenerationResult = qrcode_initText(&qrCodeInstance, qrCodeDataBuffer, qrCodeVersion4, ECC_MEDIUM, wifiString.c_str());
    
    if (qrGenerationResult != 0) {
        Serial.print("Failed to generate QR code, error: ");
        Serial.println(qrGenerationResult);
        return;
    }
    
    Serial.print("QR Code generated successfully! Size: ");
    Serial.print(qrCodeInstance.size);
    Serial.println(" modules");
    
    for (uint8_t qrModuleY = 0; qrModuleY < qrCodeInstance.size; qrModuleY++) {
        for (uint8_t qrModuleX = 0; qrModuleX < qrCodeInstance.size; qrModuleX++) {
            bool moduleIsBlack = qrcode_getModule(&qrCodeInstance, qrModuleX, qrModuleY);
            if (moduleIsBlack) {
                for (int scaledPixelY = 0; scaledPixelY < scale; scaledPixelY++) {
                    for (int scaledPixelX = 0; scaledPixelX < scale; scaledPixelX++) {
                        int displayPixelX = x + (qrModuleX * scale) + scaledPixelX;
                        int displayPixelY = y + (qrModuleY * scale) + scaledPixelY;
                        
                        bool pixelIsWithinDisplayBounds = displayPixelX < display.width() && displayPixelY < display.height();
                        if (pixelIsWithinDisplayBounds) {
                            display.drawPixel(displayPixelX, displayPixelY, GxEPD_BLACK);
                        }
                    }
                }
            }
        }
    }
    
    Serial.println("QR Code drawn to display");
}

void ConfigurationScreen::render() {
    Serial.println("Displaying configuration screen with QR code");
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont();
    
    const int textLeftMargin = 8;
    const int textLineSpacing = 16;
    const int qrCodePixelScale = 3;
    const int qrCodeModuleCount = 33;
    const int qrCodePixelSize = qrCodeModuleCount * qrCodePixelScale;
    const int qrCodeQuietZonePixels = 6;
    
    int qrCodePositionX = display.width() - qrCodePixelSize - qrCodeQuietZonePixels - 5;
    int qrCodePositionY = 10;
    
    int currentTextLineY = 15;
    int availableTextWidth = qrCodePositionX - textLeftMargin - 8;
    
    display.setCursor(textLeftMargin, currentTextLineY);
    display.println("Config Mode");
    currentTextLineY += textLineSpacing;
    
    display.setCursor(textLeftMargin, currentTextLineY);
    display.println("WiFi:");
    currentTextLineY += textLineSpacing;
    
    display.setCursor(textLeftMargin, currentTextLineY);
    display.println(accessPointName);
    currentTextLineY += textLineSpacing;
    
    display.setCursor(textLeftMargin, currentTextLineY);
    display.print("Pass: ");
    display.println(accessPointPassword);
    currentTextLineY += textLineSpacing + 5;
    
    display.setCursor(textLeftMargin, currentTextLineY);
    display.println("Scan QR:");
    
    String wifiQRCodeString = generateWiFiQRString();
    
    int qrCodeBackgroundX = qrCodePositionX - qrCodeQuietZonePixels;
    int qrCodeBackgroundY = qrCodePositionY - qrCodeQuietZonePixels;
    int qrCodeBackgroundWidth = qrCodePixelSize + (2 * qrCodeQuietZonePixels);
    int qrCodeBackgroundHeight = qrCodePixelSize + (2 * qrCodeQuietZonePixels);
    
    display.fillRect(qrCodeBackgroundX, qrCodeBackgroundY, qrCodeBackgroundWidth, qrCodeBackgroundHeight, GxEPD_WHITE);
    
    drawQRCode(wifiQRCodeString, qrCodePositionX, qrCodePositionY, qrCodePixelScale);
    
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    
    Serial.println("Configuration screen with enhanced QR code displayed");
    Serial.println("QR code optimized for iPhone scanning");
} 