#ifndef RENDERING_H
#define RENDERING_H

#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include "Weather.h"
#include <U8g2_for_Adafruit_GFX.h>

class Rendering {
private:
    GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
    U8G2_FOR_ADAFRUIT_GFX gfx;
    
    const uint8_t* primaryFont;
    const uint8_t* secondaryFont;
    const uint8_t* smallFont;
    
    int parseHHMMtoMinutes(const String& hhmm);
    void drawMeteogram(Weather& weather, int x, int y, int w, int h);
    void drawDottedLine(int x0, int y0, int x1, int y1, uint16_t color);

public:
    Rendering(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display);
    
    void displayWeather(Weather& weather);
    void displayWifiErrorIcon();
};

#endif 