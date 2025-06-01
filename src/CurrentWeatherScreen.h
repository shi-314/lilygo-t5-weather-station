#ifndef CURRENT_WEATHER_SCREEN_H
#define CURRENT_WEATHER_SCREEN_H

#include <GxEPD2_4G_4G.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>

#include "OpenMeteoAPI.h"
#include "Screen.h"

class CurrentWeatherScreen : public Screen {
 private:
  GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
  U8G2_FOR_ADAFRUIT_GFX gfx;
  const WeatherForecast& forecast;
  const String cityName;
  const String countryCode;

  const uint8_t* primaryFont;
  const uint8_t* mediumFont;
  const uint8_t* smallFont;

 public:
  CurrentWeatherScreen(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display,
                       const WeatherForecast& forecast, const String& cityName, const String& countryCode);

  void render() override;
};

#endif