#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <WiFi.h>

// Define pins for the display
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RSET 16
#define EPD_BUSY 4

// WiFi credentials
const char* ssid = ":(";
const char* password = "20009742591595504581";

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

bool wifiConnected = false;

void updateDisplay() {
    // Set text properties
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    
    // Clear the display area
    display.fillRect(30, 10, 180, 50, GxEPD_WHITE);
    
    // Display WiFi status
    display.setCursor(30, 10);
    display.print("WiFi: ");
    display.println(wifiConnected ? "Connected" : "Disconnected");
    
    if (wifiConnected) {
        display.setCursor(30, 40);
        display.print("IP: ");
        display.println(WiFi.localIP());
    }
    
    // Update only the area we changed with faster refresh
    display.updateWindow(30, 10, 180, 50, true);
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    
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
        wifiConnected = true;
    } else {
        Serial.println("\nFailed to connect to WiFi");
        wifiConnected = false;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    // Initialize SPI
    SPI.begin(18, 19, 23);  // SCK, MISO, MOSI

    // Initialize display
    display.init();
    Serial.println("Display initialized");

    // Set rotation to landscape (1 = 90 degrees clockwise)
    display.setRotation(1);
    
    // Initial full display update
    display.fillScreen(GxEPD_WHITE);
    display.update();
    
    // Connect to WiFi
    connectToWiFi();
    
    // Update display with WiFi status
    updateDisplay();
}

void loop() {
    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        wifiConnected = false;
        updateDisplay();
    } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
        wifiConnected = true;
        updateDisplay();
    }
    
    delay(1000); // Check every second
} 