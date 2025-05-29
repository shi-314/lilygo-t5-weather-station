#ifndef GEMINI_CLIENT_H
#define GEMINI_CLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class GeminiClient {
private:
    const char* apiKey;
    const char* baseUrl;
    String model;
    WiFiClientSecure client;
    HTTPClient http;
    
    String makeRequest(const String& endpoint, const String& payload);
    
public:
    GeminiClient();
    ~GeminiClient();
    
    void setModel(const String& modelName);
    String generateContent(const String& prompt);
    bool isConnected();
};

#endif 