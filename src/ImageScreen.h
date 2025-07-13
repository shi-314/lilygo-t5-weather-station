#ifndef IMAGE_SCREEN_H
#define IMAGE_SCREEN_H

#include <HTTPClient.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "DisplayType.h"
#include "Screen.h"

class ImageScreen : public Screen {
 private:
  DisplayType& display;
  U8G2_FOR_ADAFRUIT_GFX gfx;

  const uint8_t* smallFont;

  // Hard-coded configuration for the image server
  const char* imageServerUrl = "http://shivans-macbook-air.local:8080";  // Replace with your server URL

  bool downloadAndDisplayImage();
  void displayError(const String& errorMessage);
  String urlEncode(const String& str);

 public:
  ImageScreen(DisplayType& display);
  void render() override;
};

#endif