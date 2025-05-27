#include "GeminiClient.h"

// Replace with your actual API key
const char* GEMINI_API_KEY = "AIzaSyCEipBSXv5R2GChphaT9UFsirSZazJcumQ";
const char* GEMINI_BASE_URL = "https://generativelanguage.googleapis.com";

GeminiClient::GeminiClient() {
    apiKey = GEMINI_API_KEY;
    baseUrl = GEMINI_BASE_URL;
}

GeminiClient::~GeminiClient() {
    http.end();
}

bool GeminiClient::initialize() {
    client.setInsecure();
    return true;
}

String GeminiClient::makeRequest(const String& endpoint, const String& payload) {
    String url = String(baseUrl) + endpoint + "?key=" + String(apiKey);
    
    Serial.println("Request URL: " + url);
    Serial.println("Payload size: " + String(payload.length()) + " bytes");
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    
    http.setTimeout(30000); // 30 second timeout
    
    int httpResponseCode = http.POST(payload);
    String response = "";
    
    if (httpResponseCode > 0) {
        response = http.getString();
        Serial.println("HTTP Response Code: " + String(httpResponseCode));
        Serial.println("Response: " + response);
    } else {
        Serial.println("Error in HTTP request: " + String(httpResponseCode));
        Serial.println("Error details:");
        switch(httpResponseCode) {
            case -1: Serial.println("Connection failed"); break;
            case -2: Serial.println("Send header failed"); break;
            case -3: Serial.println("Send payload failed"); break;
            case -4: Serial.println("Not connected"); break;
            case -5: Serial.println("Connection lost"); break;
            case -6: Serial.println("No stream"); break;
            case -7: Serial.println("No HTTP server"); break;
            case -8: Serial.println("Too less RAM"); break;
            case -9: Serial.println("Encoding error"); break;
            case -10: Serial.println("Stream write error"); break;
            case -11: Serial.println("Read timeout"); break;
            default: Serial.println("Unknown error");
        }
    }
    
    http.end();
    return response;
}

String GeminiClient::generateContent(const String& prompt) {
    return generateContent("gemini-1.5-flash", prompt);
}

String GeminiClient::generateContent(const String& model, const String& prompt) {
    String endpoint = "/v1beta/models/" + model + ":generateContent";
    
    if (prompt.length() == 0) {
        Serial.println("Error: Empty prompt");
        return "Error: Empty prompt";
    }
    
    DynamicJsonDocument doc(8192);
    JsonArray contents = doc.createNestedArray("contents");
    JsonObject content = contents.createNestedObject();
    content["role"] = "user";
    JsonArray parts = content.createNestedArray("parts");
    JsonObject part = parts.createNestedObject();
    part["text"] = prompt;
    
    String payload;
    serializeJson(doc, payload);
    
    String response = makeRequest(endpoint, payload);
    
    DynamicJsonDocument responseDoc(4096);
    deserializeJson(responseDoc, response);
    
    if (responseDoc.containsKey("candidates") && 
        responseDoc["candidates"].size() > 0 &&
        responseDoc["candidates"][0].containsKey("content") &&
        responseDoc["candidates"][0]["content"].containsKey("parts") &&
        responseDoc["candidates"][0]["content"]["parts"].size() > 0) {
        
        return responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
    }

    Serial.println("Error: Could not parse response");
    Serial.println(response);
    
    return "Error: Could not parse response";
}

bool GeminiClient::isConnected() {
    return WiFi.status() == WL_CONNECTED;
} 