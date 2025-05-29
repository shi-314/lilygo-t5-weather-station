#ifndef CHATGPT_CLIENT_H
#define CHATGPT_CLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class ChatGPTClient {
private:
    const char* apiKey;
    const char* baseUrl;
    String model;
    WiFiClientSecure client;
    HTTPClient http;
    
    String makeRequest(const String& endpoint, const String& payload);
    String base64ToByteArray(const String& base64String);
    
public:
    ChatGPTClient();
    ~ChatGPTClient();
    
    void setModel(const String& modelName);
    String generateContent(const String& prompt);
    String generateImage(const String& prompt);
};

#endif 