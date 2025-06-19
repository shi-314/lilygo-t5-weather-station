#pragma once

#include <Arduino.h>

struct ApplicationConfig {
  char wifiSSID[64];
  char wifiPassword[64];
  char openaiApiKey[200];
  char aiPromptStyle[200];
  char city[100];
  char countryCode[3];
  float latitude;
  float longitude;

  ApplicationConfig() {
    memset(wifiSSID, 0, sizeof(wifiSSID));
    memset(wifiPassword, 0, sizeof(wifiPassword));
    memset(openaiApiKey, 0, sizeof(openaiApiKey));
    memset(aiPromptStyle, 0, sizeof(aiPromptStyle));
    memset(city, 0, sizeof(city));
    memset(countryCode, 0, sizeof(countryCode));
    latitude = NAN;
    longitude = NAN;
  }

  bool hasValidWiFiCredentials() const { return strlen(wifiSSID) > 0 && strlen(wifiPassword) > 0; }

  bool hasValidOpenaiApiKey() const { return strlen(openaiApiKey) > 0; }
};