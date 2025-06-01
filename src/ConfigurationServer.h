#ifndef CONFIGURATION_SERVER_H
#define CONFIGURATION_SERVER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include <functional>

using OnSaveCallback = std::function<void(const String &ssid, const String &password, const String &openaiApiKey,
                                          const String &aiPromptStyle)>;

class ConfigurationServer {
 public:
  ConfigurationServer(const char *currentSSID, const char *currentPassword, const char *currentOpenaiKey,
                      const char *currentAiPromptStyle);
  void run(OnSaveCallback onSaveCallback);
  void stop();
  bool isRunning() const;
  void handleRequests();

  String getWifiAccessPointName() const;
  String getWifiAccessPointPassword() const;

 private:
  String deviceName;
  String wifiAccessPointName;
  String wifiAccessPointPassword;

  const char *currentWifiSSID;
  const char *currentWifiPassword;
  const char *currentOpenaiApiKey;
  const char *currentAiPromptStyle;

  AsyncWebServer *server;
  DNSServer *dnsServer;
  bool isServerRunning;

  String htmlTemplate;
  OnSaveCallback onSaveCallback;

  void setupWebServer();
  void setupDNSServer();
  String getConfigurationPage();
  bool loadHtmlTemplate();
  void handleRoot(AsyncWebServerRequest *request);
  void handleSave(AsyncWebServerRequest *request);
  void handleNotFound(AsyncWebServerRequest *request);
};

#endif