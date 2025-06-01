#ifndef RENDERING_H
#define RENDERING_H

#include <GxEPD2_4G_4G.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>

#include "OpenMeteoAPI.h"
#include "Screen.h"

class MeteogramWeatherScreen : public Screen {
 private:
  GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
  U8G2_FOR_ADAFRUIT_GFX gfx;
  OpenMeteoAPI& weather;

  const uint8_t* primaryFont;
  const uint8_t* secondaryFont;
  const uint8_t* smallFont;

  int parseHHMMtoMinutes(const String& hhmm);
  void drawMeteogram(int x, int y, int w, int h);
  void drawDottedLine(int x0, int y0, int x1, int y1, uint16_t color);

 public:
  MeteogramWeatherScreen(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display,
                         OpenMeteoAPI& weather);

  void render() override;
};

#endif