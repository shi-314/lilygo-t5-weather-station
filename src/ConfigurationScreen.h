#ifndef CONFIGURATION_SCREEN_H
#define CONFIGURATION_SCREEN_H

#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <qrcode.h>

#include "Screen.h"

class ConfigurationScreen : public Screen {
 private:
  GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display;
  String accessPointName;
  String accessPointPassword;

  void drawQRCode(const String& wifiString, int x, int y, int scale = 2);
  String generateWiFiQRString() const;

 public:
  ConfigurationScreen(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display,
                      const String& accessPointName, const String& accessPointPassword);

  void render() override;
};

#endif