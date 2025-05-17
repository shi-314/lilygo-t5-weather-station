#include "battery.h"

String getBatteryStatus() {
    int rawValue = analogRead(BATTERY_PIN);
    // Convert to voltage (ESP32 ADC is 12-bit, 0-3.3V)
    float voltage = (rawValue * 3.3) / 4095.0;  
    // Adjust for voltage divider if used
    voltage *= VOLTAGE_DIVIDER_RATIO;  
    
    int percentage = map(voltage * 100, BATTERY_MIN_VOLTAGE * 100, BATTERY_MAX_VOLTAGE * 100, 0, 100);

    // Ensure percentage is between 0 and 100
    percentage = constrain(percentage, 0, 100);  
    
    return String(percentage) + "%";
} 