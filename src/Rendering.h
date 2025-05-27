#ifndef RENDERING_H
#define RENDERING_H

#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include "Weather.h"
#include <U8g2_for_Adafruit_GFX.h>

class Rendering {
private:
    GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
    U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
    
    // Sans serif font options - you can change these to different U8g2 sans serif fonts:
    // 
    // HELVETICA (Classic sans serif):
    // Primary: u8g2_font_helvB14_tr, u8g2_font_helvB18_tr, u8g2_font_helvB12_tr
    // Secondary: u8g2_font_helvR10_tr, u8g2_font_helvR12_tr, u8g2_font_helvR14_tr
    // Small: u8g2_font_helvR08_tr, u8g2_font_helvR09_tr
    //
    // PROFONT (Clean programming font):
    // Primary: u8g2_font_profont15_tr, u8g2_font_profont17_tr
    // Secondary: u8g2_font_profont12_tr, u8g2_font_profont11_tr
    // Small: u8g2_font_profont10_tr
    //
    // SIMPLE SANS SERIF:
    // Primary: u8g2_font_9x15B_tr, u8g2_font_10x20_tr
    // Secondary: u8g2_font_8x13B_tr, u8g2_font_9x15_tr
    // Small: u8g2_font_6x10_tr, u8g2_font_5x7_tr
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