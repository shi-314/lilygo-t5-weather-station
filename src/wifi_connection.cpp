#include "wifi_connection.h"
#include <WiFi.h>

WiFiConnection::WiFiConnection(const char* ssid, const char* password)
    : _ssid(ssid), _password(password), connected(false) {}

void WiFiConnection::connect() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(_ssid, _password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        connected = true;
    } else {
        Serial.println("\nFailed to connect to WiFi");
        connected = false;
    }
}

void WiFiConnection::reconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Attempting to reconnect to WiFi...");
        WiFi.disconnect();
        delay(1000);
        connect();
    }
}

bool WiFiConnection::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiConnection::checkConnection() {
    if (WiFi.status() != WL_CONNECTED && connected) {
        Serial.println("WiFi connection lost");
        connected = false;
        reconnect();
    } else if (WiFi.status() == WL_CONNECTED && !connected) {
        Serial.println("WiFi reconnected");
        connected = true;
    }
} 