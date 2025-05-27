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
    client.setInsecure(); // For simplicity, skip certificate validation
    return true;
}

String GeminiClient::makeRequest(const String& endpoint, const String& payload) {
    String url = String(baseUrl) + endpoint + "?key=" + String(apiKey);
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(payload);
    String response = "";
    
    if (httpResponseCode > 0) {
        response = http.getString();
        // Serial.println("HTTP Response Code: " + String(httpResponseCode));
        // Serial.println("Response: " + response);
    } else {
        Serial.println("Error in HTTP request: " + String(httpResponseCode));
    }
    
    http.end();
    return response;
}

String GeminiClient::generateContent(const String& prompt) {
    return generateContent("gemini-1.5-flash", prompt);
}

String GeminiClient::generateContent(const String& model, const String& prompt) {
    String endpoint = "/v1beta/models/" + model + ":generateContent";
    
    DynamicJsonDocument doc(2048);
    JsonArray contents = doc.createNestedArray("contents");
    JsonObject content = contents.createNestedObject();
    JsonArray parts = content.createNestedArray("parts");
    JsonObject part = parts.createNestedObject();
    part["text"] = prompt;
    
    String payload;
    serializeJson(doc, payload);
    
    String response = makeRequest(endpoint, payload);
    
    // Parse response to extract generated text
    DynamicJsonDocument responseDoc(4096);
    deserializeJson(responseDoc, response);
    
    if (responseDoc.containsKey("candidates") && 
        responseDoc["candidates"].size() > 0 &&
        responseDoc["candidates"][0].containsKey("content") &&
        responseDoc["candidates"][0]["content"].containsKey("parts") &&
        responseDoc["candidates"][0]["content"]["parts"].size() > 0) {
        
        return responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
    }
    
    return "Error: Could not parse response";
}

bool GeminiClient::isConnected() {
    return WiFi.status() == WL_CONNECTED;
} 