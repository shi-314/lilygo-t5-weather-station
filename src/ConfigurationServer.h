#ifndef CONFIGURATION_SERVER_H
#define CONFIGURATION_SERVER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

class ConfigurationServer {
 public:
  ConfigurationServer();
  void run();
  void stop();
  void handleRequests();

  String getWifiAccessPointName() const;
  String getWifiAccessPointPassword() const;

 private:
  String deviceName;
  String wifiAccessPointName;
  String wifiAccessPointPassword;

  AsyncWebServer *server;
  DNSServer *dnsServer;
  bool isRunning;

  String htmlTemplate;  // Cached HTML template

  void setupWebServer();
  void setupDNSServer();
  String getConfigurationPage();
  bool loadHtmlTemplate();  // Returns success/failure, loads once
  void handleRoot(AsyncWebServerRequest *request);
  void handleSave(AsyncWebServerRequest *request);
  void handleNotFound(AsyncWebServerRequest *request);
};

#endif