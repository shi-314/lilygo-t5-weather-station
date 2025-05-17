#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// Define pins for the display
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RSET 16
#define EPD_BUSY 4

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    // Initialize SPI
    SPI.begin(18, 19, 23);  // SCK, MISO, MOSI

    // Initialize display
    display.init();
    Serial.println("Display initialized");

    // Clear the display
    display.fillScreen(GxEPD_WHITE);
    
    // Set text properties
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    
    // Draw some text
    display.setCursor(10, 30);
    display.println("Hello World!");
    
    // Update the display
    display.update();
    Serial.println("Display updated");
}

void loop() {
    // Do nothing
    delay(1000);
} 