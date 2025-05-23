#include "Rendering.h"
#include "battery.h"
#include <Fonts/FreeSans9pt7b.h>  // Smaller font for general text
#include <Fonts/FreeSans12pt7b.h> // Larger font for weather data
#include "Fonts/Picopixel.h" // Font for meteogram labels
#include "assets/WifiErrorIcon.h"
#include <vector>
#include <algorithm>

Rendering::Rendering(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display) 
    : display(display) {
}

int Rendering::parseHHMMtoMinutes(const String& hhmm) {
    if (hhmm.length() != 5 || hhmm.charAt(2) != ':') {
        return -1; // Invalid format
    }
    int hours = hhmm.substring(0, 2).toInt();
    int minutes = hhmm.substring(3, 5).toInt();
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
        return -1; // Invalid time
    }
    return hours * 60 + minutes;
}

void Rendering::drawWindDirectionIndicator(int x, int y, int radius, int direction) {
    display.drawCircle(x, y, radius, GxEPD_LIGHTGREY);
    
    // Calculate the end point of the direction line
    // Meteorological wind direction: 0° = wind FROM North, 90° = wind FROM East
    // We need to:
    // 1. Convert from "wind from" to "wind to" by adding 180°
    // 2. Adjust for screen coordinates where Y increases downward
    
    // Convert from "wind from" to "wind to"
    int adjustedDir = (direction + 180) % 360;
    
    // In screen coordinates: 0° is right, clockwise rotation
    // North is upward (270°), East is right (0°), South is down (90°), West is left (180°)
    float angleRadians = adjustedDir * PI / 180.0;
    
    // Calculate end point (note: sin is negated because screen Y increases downward)
    int endX = x + round(radius * sin(angleRadians));
    int endY = y - round(radius * cos(angleRadians));
    
    // Draw the direction line
    display.drawLine(x, y, endX, endY, GxEPD_DARKGREY);
    
    // Draw a small circle at the center for better visibility
    display.fillCircle(x, y, 2, GxEPD_BLACK);
}

void Rendering::displayWeather(Weather& weather) {
    Serial.println("Updating display...");
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);

    // Display Battery Status at top right
    display.setFont(nullptr);
    display.setTextColor(GxEPD_LIGHTGREY);
    String batteryStatus = getBatteryStatus();
    int16_t x1_batt, y1_batt;
    uint16_t w_batt, h_batt;
    display.getTextBounds(batteryStatus, 0, 0, &x1_batt, &y1_batt, &w_batt, &h_batt);
    display.setCursor(display.width() - w_batt - 3, 6);
    display.print(batteryStatus);

    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans12pt7b);

    // Position current weather and wind text higher and more compactly
    int current_weather_y = 24; 
    display.setCursor(6, current_weather_y);
    
    String temperatureDisplay = String(weather.getCurrentTemperature(), 1) + " C";
    display.print(temperatureDisplay);
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(temperatureDisplay, 0, 0, &x1, &y1, &w, &h);
    
    display.setFont(&FreeSans9pt7b);
    display.setCursor(6 + w + 8, current_weather_y); // Add some spacing between temp and description
    display.print(" " + weather.getWeatherDescription());
    
    display.setFont(&FreeSans12pt7b);

    int wind_text_y = current_weather_y + 24;
    display.setCursor(6, wind_text_y);
    
    String windDisplay = String(weather.getCurrentWindSpeed(), 1) + " km/h";
    display.println(windDisplay);
    
    // Draw wind direction indicator, aligned with wind text
    int radius = 12; 
    int windDirX = display.width() - radius - 6;
    int windDirY = y1_batt + h_batt + 24;
    drawWindDirectionIndicator(windDirX, windDirY, radius, weather.getWindDirection());

    // Define Meteogram Area (below current weather, using more height)
    int meteogramX = 0;
    int meteogramY = wind_text_y + 10;
    int meteogramW = display.width();
    int meteogramH = display.height() - meteogramY - 3; 
    drawMeteogram(weather, meteogramX, meteogramY, meteogramW, meteogramH);
    
    // Update the entire display
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    Serial.println("Display updated");
}

void Rendering::drawMeteogram(Weather& weather, int x_base, int y_base, int w, int h) {
    display.setFont(nullptr);
    std::vector<float> temps = weather.getHourlyTemperatures();
    std::vector<float> winds = weather.getHourlyWindSpeeds();
    std::vector<String> times = weather.getHourlyTime();

    if (temps.empty() || winds.empty() || times.empty()) {
        display.setCursor(x_base, y_base + h / 2);
        display.print("No meteogram data.");
        return;
    }

    int num_points = std::min({(int)temps.size(), (int)winds.size(), (int)times.size(), 24});
    if (num_points <= 1) { 
        display.setCursor(x_base, y_base + h / 2);
        display.print("Not enough data.");
        return;
    }

    // Find min/max values
    float min_temp = *std::min_element(temps.begin(), temps.begin() + num_points);
    float max_temp = *std::max_element(temps.begin(), temps.begin() + num_points);
    float min_wind = *std::min_element(winds.begin(), winds.begin() + num_points);
    float max_wind = *std::max_element(winds.begin(), winds.begin() + num_points);
    
    // Ensure we have a range for scaling
    if (max_temp == min_temp) max_temp += 1.0f;
    if (max_wind == min_wind) max_wind += 1.0f;

    // Calculate label dimensions
    int16_t x1, y1;
    uint16_t label_w, label_h;
    display.getTextBounds("99", 0, 0, &x1, &y1, &label_w, &label_h);
    
    int left_padding = label_w + 6;   // Space for temperature labels
    int right_padding = label_w + 6;  // Space for wind labels  
    int bottom_padding = label_h + 6; // Space for time label

    // Define plot area
    int plot_x = x_base + left_padding;
    int plot_y = y_base;
    int plot_w = w - left_padding - right_padding;
    int plot_h = h - bottom_padding;

    if (plot_w <= 20 || plot_h <= 10) {
        display.setCursor(x_base, y_base + h / 2);
        display.print("Too small for graph.");
        return; 
    }

    // Draw plot border
    display.drawRect(plot_x, plot_y, plot_w, plot_h, GxEPD_LIGHTGREY);
    
    float x_step = (float)plot_w / (num_points - 1);
    display.setTextColor(GxEPD_BLACK);

    // Draw Y-axis labels
    String temp_labels[] = {String(max_temp, 0), String(min_temp, 0)};
    String wind_labels[] = {String(max_wind, 0), String(min_wind, 0)};
    int y_positions[] = {plot_y, plot_y + plot_h};
    
    for (int i = 0; i < 2; i++) {
        // Temperature labels (left)
        display.getTextBounds(temp_labels[i], 0, 0, &x1, &y1, &label_w, &label_h);
        display.setCursor(plot_x - label_w - 3, y_positions[i]);
        display.print(temp_labels[i]);
        
        // Wind labels (right)
        display.setCursor(plot_x + plot_w + 3, y_positions[i]);
        display.print(wind_labels[i]);
    }

    // Plot both Temperature and Wind Speed lines
    for (int i = 0; i < num_points - 1; ++i) {
        int x1 = plot_x + round(i * x_step);
        int x2 = plot_x + round((i + 1) * x_step);
        
        // Calculate temperature line points
        int y1_temp = plot_y + plot_h - round(((temps[i] - min_temp) / (max_temp - min_temp)) * plot_h);
        int y2_temp = plot_y + plot_h - round(((temps[i+1] - min_temp) / (max_temp - min_temp)) * plot_h);
        
        // Calculate wind speed line points  
        int y1_wind = plot_y + plot_h - round(((winds[i] - min_wind) / (max_wind - min_wind)) * plot_h);
        int y2_wind = plot_y + plot_h - round(((winds[i+1] - min_wind) / (max_wind - min_wind)) * plot_h);
        
        // Clip to bounds and draw lines
        display.drawLine(x1, constrain(y1_temp, plot_y, plot_y + plot_h), 
                        x2, constrain(y2_temp, plot_y, plot_y + plot_h), GxEPD_BLACK);
        display.drawLine(x1, constrain(y1_wind, plot_y, plot_y + plot_h), 
                        x2, constrain(y2_wind, plot_y, plot_y + plot_h), GxEPD_DARKGREY);
    }

    // Draw vertical line and last update time
    String lastUpdateStr = weather.getLastUpdateTime();
    int lastUpdateMinutes = parseHHMMtoMinutes(lastUpdateStr);
    int final_line_x = -1;

    if (lastUpdateMinutes != -1 && num_points > 1) {
        // Find the position for the last update time
        for (int i = 0; i < num_points; ++i) {
            int currentMinutes = parseHHMMtoMinutes(times[i]);
            if (currentMinutes == -1) continue;
            
            // Check if we found an exact match
            if (lastUpdateMinutes == currentMinutes) {
                final_line_x = plot_x + round(i * x_step);
                break;
            }
            
            // Check if we're between two points
            if (i < num_points - 1) {
                int nextMinutes = parseHHMMtoMinutes(times[i + 1]);
                if (nextMinutes != -1 && 
                    lastUpdateMinutes > currentMinutes && 
                    lastUpdateMinutes < nextMinutes) {
                    // Interpolate between the two points
                    float fraction = (float)(lastUpdateMinutes - currentMinutes) / (nextMinutes - currentMinutes);
                    final_line_x = plot_x + round((i + fraction) * x_step);
                    break;
                }
            }
        }
    }

    if (final_line_x != -1 && final_line_x >= plot_x && final_line_x <= plot_x + plot_w) {
        display.drawFastVLine(final_line_x, plot_y, plot_h, GxEPD_BLACK);

        // Draw the last update time string below the line
        display.getTextBounds(lastUpdateStr, 0, 0, &x1, &y1, &label_w, &label_h);
        int time_x = constrain(final_line_x - label_w / 2, x_base, x_base + w - label_w);
        display.setCursor(time_x, plot_y + plot_h + bottom_padding - 6);
        display.print(lastUpdateStr);
    }
}

void Rendering::displayWifiErrorIcon() {
    Serial.println("Displaying WiFi error icon");
    
    display.init(115200);
    display.setRotation(1);  // Ensure consistent rotation
    display.fillScreen(GxEPD_WHITE);
    
    // WifiErrorIcon is 16x16 pixels
    int iconWidth = 16;
    int iconHeight = 16;
    
    // Calculate center position to place the icon exactly in the center
    int centerX = (display.width() / 2) - (iconWidth / 2);
    int centerY = (display.height() / 2) - (iconHeight / 2);
    
    // Draw the icon at the center
    display.drawBitmap(centerX, centerY, WifiErrorIcon, iconWidth, iconHeight, GxEPD_BLACK);
    
    // Full update to ensure the icon is properly displayed
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
} 