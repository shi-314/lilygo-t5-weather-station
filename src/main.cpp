#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Define pins for the display
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RSET 16
#define EPD_BUSY 4

// Define pin for the button
#define BUTTON_PIN 39

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

// Global variables
int counter = 0;
bool displayNeedsUpdate = true;
QueueHandle_t buttonQueue;

// Button task
void buttonTask(void *pvParameters) {
    bool lastButtonState = HIGH;
    
    while(1) {
        bool currentButtonState = digitalRead(BUTTON_PIN);
        
        // Detect button press (transition from HIGH to LOW)
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            // Send message to queue
            xQueueSend(buttonQueue, &counter, 0);
        }
        
        lastButtonState = currentButtonState;
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent CPU hogging
    }
}

void updateDisplay() {
    // Set text properties
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    
    // Calculate the exact area needed for the number
    // Assuming max 3 digits (0-999) plus "Count: " text
    display.fillRect(30, 10, 180, 25, GxEPD_WHITE);
    
    // Display counter
    display.setCursor(30, 10);
    display.print("Count: ");
    display.println(counter);
    
    // Update only the area we changed with faster refresh
    display.updateWindow(30, 10, 180, 25, true);
    displayNeedsUpdate = false;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    // Initialize SPI
    SPI.begin(18, 19, 23);  // SCK, MISO, MOSI

    // Initialize button
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Create queue for button events
    buttonQueue = xQueueCreate(10, sizeof(int));

    // Create button task
    xTaskCreate(
        buttonTask,    // Task function
        "ButtonTask",  // Task name
        2000,         // Stack size
        NULL,         // Task parameters
        1,            // Task priority
        NULL          // Task handle
    );

    // Initialize display
    display.init();
    Serial.println("Display initialized");

    // Set rotation to landscape (1 = 90 degrees clockwise)
    display.setRotation(1);
    
    // Initial full display update
    display.fillScreen(GxEPD_WHITE);
    display.update();
    
    // Initial counter display
    updateDisplay();
}

void loop() {
    int receivedValue;
    
    // Check for button events
    if (xQueueReceive(buttonQueue, &receivedValue, 0) == pdTRUE) {
        counter++;
        displayNeedsUpdate = true;
    }
    
    // Update display if needed
    if (displayNeedsUpdate) {
        updateDisplay();
    }
    
    delay(5); // Reduced main loop delay
} 