#ifndef RENDERING_H
#define RENDERING_H

#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include "Weather.h"
#include <gfxfont.h>

class Rendering {
private:
    GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
    
    const GFXfont* primaryFont;
    const GFXfont* secondaryFont;
    const GFXfont* smallFont;
    
    int parseHHMMtoMinutes(const String& hhmm);
    void drawMeteogram(Weather& weather, int x, int y, int w, int h);
    void drawDottedLine(int x0, int y0, int x1, int y1, uint16_t color);

public:
    Rendering(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display);
    
    void displayWeather(Weather& weather);
    void displayWifiErrorIcon();
};

#endif 