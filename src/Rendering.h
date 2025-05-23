#ifndef RENDERING_H
#define RENDERING_H

#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include "Weather.h"

class Rendering {
private:
    GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
    
    int parseHHMMtoMinutes(const String& hhmm);
    void drawMeteogram(Weather& weather, int x, int y, int w, int h);

public:
    Rendering(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display);
    
    void displayWeather(Weather& weather);
    void displayWifiErrorIcon();
};

#endif 