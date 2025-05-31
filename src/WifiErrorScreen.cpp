#include "WifiErrorScreen.h"

#include "assets/WifiErrorIcon.h"

WifiErrorScreen::WifiErrorScreen(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display)
    : display(display) {}

void WifiErrorScreen::render() {
  Serial.println("Displaying WiFi error icon");

  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);

  int iconWidth = 16;
  int iconHeight = 16;

  int centerX = (display.width() / 2) - (iconWidth / 2);
  int centerY = (display.height() / 2) - (iconHeight / 2);

  display.drawBitmap(centerX, centerY, WifiErrorIcon, iconWidth, iconHeight, GxEPD_BLACK);

  display.displayWindow(0, 0, display.width(), display.height());
  display.hibernate();
}