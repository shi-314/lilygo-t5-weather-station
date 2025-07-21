#ifndef IMAGE_SCREEN_H
#define IMAGE_SCREEN_H

#include <HTTPClient.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "ApplicationConfig.h"
#include "DisplayType.h"
#include "Screen.h"

class ImageScreen : public Screen {
 private:
  DisplayType& display;
  U8G2_FOR_ADAFRUIT_GFX gfx;
  ApplicationConfig& config;

  const uint8_t* smallFont;

  const char* imageServerUrl = "https://dither.lab.shvn.dev";

  bool downloadAndDisplayImage();
  void displayError(const String& errorMessage);
  String urlEncode(const String& str);
  String buildImageUrl();

 public:
  ImageScreen(DisplayType& display, ApplicationConfig& config);
  void render() override;
};

#endif