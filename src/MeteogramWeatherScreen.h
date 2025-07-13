#ifndef RENDERING_H
#define RENDERING_H

#include <U8g2_for_Adafruit_GFX.h>

#include "DisplayType.h"
#include "OpenMeteoAPI.h"
#include "Screen.h"

class MeteogramWeatherScreen : public Screen {
 private:
  DisplayType& display;
  U8G2_FOR_ADAFRUIT_GFX gfx;
  WeatherForecast forecast;

  const uint8_t* primaryFont;
  const uint8_t* secondaryFont;
  const uint8_t* smallFont;

  int parseHHMMtoMinutes(const String& hhmm);
  void drawMeteogram(int x, int y, int w, int h);
  void drawDottedLine(int x0, int y0, int x1, int y1, uint16_t color);

 public:
  MeteogramWeatherScreen(DisplayType& display, const WeatherForecast& forecast);

  void render() override;
};

#endif