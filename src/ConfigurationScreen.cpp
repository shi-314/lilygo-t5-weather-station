#include "ConfigurationScreen.h"

ConfigurationScreen::ConfigurationScreen(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display,
                                         const String& accessPointName, const String& accessPointPassword)
    : display(display),
      accessPointName(accessPointName),
      accessPointPassword(accessPointPassword)
{
}

void ConfigurationScreen::render() {
    Serial.println("Displaying configuration screen");
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont();
    
    int currentLineY = 30;
    const int lineSpacing = 20;
    const int leftMargin = 10;
    
    display.setCursor(leftMargin, currentLineY);
    display.println("Configuration Mode");
    currentLineY += lineSpacing;
    
    display.setCursor(leftMargin, currentLineY);
    display.println("Connect to WiFi:");
    currentLineY += lineSpacing;
    
    display.setCursor(leftMargin, currentLineY);
    display.println(accessPointName);
    currentLineY += lineSpacing;
    
    display.setCursor(leftMargin, currentLineY);
    display.print("Password: ");
    display.println(accessPointPassword);
    
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    
    Serial.println("Configuration screen displayed");
} 