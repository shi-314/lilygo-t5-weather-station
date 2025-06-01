#include "CurrentWeatherScreen.h"

#include "battery.h"

CurrentWeatherScreen::CurrentWeatherScreen(
    GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display, const WeatherForecast& forecast,
    const String& cityName, const String& countryCode)
    : display(display),
      forecast(forecast),
      cityName(cityName),
      countryCode(countryCode),
      primaryFont(u8g2_font_helvB24_tf),
      smallFont(u8g2_font_helvR08_tr) {
  gfx.begin(display);
}

void CurrentWeatherScreen::render() {
  Serial.println("Displaying current weather screen");

  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);

  gfx.setFontMode(1);
  gfx.setForegroundColor(GxEPD_BLACK);
  gfx.setBackgroundColor(GxEPD_WHITE);

  if (forecast.lastUpdateTime.length() == 0) {
    Serial.println("Warning: Forecast data is invalid, displaying error message");
    gfx.setFont(smallFont);
    gfx.setCursor(10, 30);
    gfx.print("Weather data unavailable");
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    return;
  }

  int topMargin = 5;
  int usableHeight = display.height() - topMargin;

  gfx.setFont(primaryFont);
  String temperatureText = String(forecast.currentTemperature, 1) + "°C";
  int temperatureWidth = gfx.getUTF8Width(temperatureText.c_str());
  int temperatureHeight = gfx.getFontAscent() - gfx.getFontDescent();
  int temperatureX = (display.width() - temperatureWidth) / 2;
  int temperatureY = topMargin + (usableHeight - temperatureHeight) / 2 + gfx.getFontAscent();

  gfx.setCursor(temperatureX, temperatureY);
  gfx.print(temperatureText);

  gfx.setFont(smallFont);
  int smallFontHeight = gfx.getFontAscent() - gfx.getFontDescent();

  gfx.setCursor(2, topMargin + smallFontHeight + 2);
  gfx.print(forecast.lastUpdateTime);

  String locationText = cityName;
  if (countryCode.length() > 0) {
    locationText += ", " + countryCode;
  }

  int locationWidth = gfx.getUTF8Width(locationText.c_str());
  gfx.setCursor(display.width() - locationWidth - 2, topMargin + smallFontHeight + 2);
  gfx.print(locationText);

  String windText = String(forecast.currentWindSpeed, 1) + " - " + String(forecast.currentWindGusts, 1) + " m/s";
  gfx.setCursor(2, display.height() - 2);
  gfx.print(windText);

  String batteryStatus = getBatteryStatus();
  int batteryWidth = gfx.getUTF8Width(batteryStatus.c_str());
  gfx.setCursor(display.width() - batteryWidth - 2, display.height() - 2);
  gfx.print(batteryStatus);

  display.displayWindow(0, 0, display.width(), display.height());
  display.hibernate();
  Serial.println("Current weather display updated");
}