#ifndef MESSAGE_SCREEN_H
#define MESSAGE_SCREEN_H

#include <GxEPD2_4G_4G.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>

#include "Screen.h"

class MessageScreen : public Screen {
 private:
  GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
  U8G2_FOR_ADAFRUIT_GFX gfx;
  String messageText;

  const uint8_t* primaryFont;
  const uint8_t* smallFont;

 public:
  MessageScreen(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display);

  void setMessageText(const String& text);
  void render() override;
};

#endif