#ifndef FIRMWARE_UPDATE_SCREEN_H
#define FIRMWARE_UPDATE_SCREEN_H

#include <U8g2_for_Adafruit_GFX.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "DisplayType.h"
#include "Screen.h"

struct FirmwareInfo {
  String currentVersion;
  String latestVersion;
  bool updateAvailable;
  String updateUrl;
  String releaseNotes;
};

class FirmwareUpdateScreen : public Screen {
 private:
  DisplayType& display;
  U8G2_FOR_ADAFRUIT_GFX gfx;
  FirmwareInfo firmwareInfo;
  
  const uint8_t* primaryFont;
  const uint8_t* mediumFont;
  const uint8_t* smallFont;

  // GitHub API settings
  const char* githubApiUrl = "https://api.github.com/repos/shi-314/lilygo-t5-weather-station/releases/latest";
  const char* currentFirmwareVersion = "1.0.0"; // This should match your current version
  
  bool checkForUpdates();
  bool compareVersions(const String& current, const String& latest);

 public:
  FirmwareUpdateScreen(DisplayType& display);
  
  void render() override;
  int nextRefreshInSeconds() override;
};

#endif