#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <time.h>
#include "battery.h"
#include "wifi_connection.h"
#include "Weather.h"
#include <Fonts/FreeSans9pt7b.h>  // Smaller font for general text
#include <Fonts/FreeSans12pt7b.h> // Larger font for weather data
#include "Fonts/Picopixel.h" // Font for meteogram labels
#include "boards.h"  // Include for LED pin definition
#include "assets/WifiErrorIcon.h"

// WiFi credentials
const char* ssid = ":(";
const char* password = "20009742591595504581";

// Location coordinates
const float latitude = 52.520008;  // Berlin latitude
const float longitude = 13.404954; // Berlin longitude

GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(GxEPD2_213_GDEY0213B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

const unsigned long sleepTime = 300000000; // Deep sleep time in microseconds (5 minutes)

WiFiConnection wifi(ssid, password);
Weather weather(latitude, longitude);

// Forward declarations
void displayWeather(Weather& weather);
void goToSleep();
void displayWifiErrorIcon();
void drawWindDirectionIndicator(int x, int y, int radius, int direction);
void drawMeteogram(Weather& weather, int x, int y, int w, int h);

// Helper function to parse HH:MM string to minutes from midnight
int parseHHMMtoMinutes(const String& hhmm) {
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

void drawWindDirectionIndicator(int x, int y, int radius, int direction) {
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

void displayWeather(Weather& weather) {
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
    display.println(weather.getWeatherText());
    
    int wind_text_y = current_weather_y + 24;
    display.setCursor(6, wind_text_y);
    display.println(weather.getWindText());
    
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

void drawMeteogram(Weather& weather, int x_base, int y_base, int w, int h) {
    // Attempt to use a very small font if available, otherwise default to 9pt
    const GFXfont* labelFont = nullptr; // Use the default small font
    display.setFont(labelFont);
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

    // Define padding for labels
    uint16_t temp_label_max_width = 0;
    uint16_t wind_label_max_width = 0;
    uint16_t text_h_val; 
    int16_t x1_bounds_val, y1_bounds_val; 
    uint16_t w_val;
    
    // Pre-calculate max label widths for precise padding
    String temp_max_str_for_width = String(temps[0],0); // Placeholder for width calc
    display.getTextBounds(temp_max_str_for_width, 0, 0, &x1_bounds_val, &y1_bounds_val, &w_val, &text_h_val);
    temp_label_max_width = w_val + 3; // text width + 3px padding

    String wind_max_str_for_width = String(winds[0],0); // Placeholder
    display.getTextBounds(wind_max_str_for_width, 0, 0, &x1_bounds_val, &y1_bounds_val, &w_val, &text_h_val);
    wind_label_max_width = w_val + 3; // text width + 3px padding

    int y_label_padding_left = temp_label_max_width;
    int y_label_padding_right = wind_label_max_width;
    int x_label_padding_bottom = text_h_val + 3; // text height + 3px padding

    int plot_x = x_base + y_label_padding_left;
    int plot_y = y_base;
    int plot_w = w - y_label_padding_left - y_label_padding_right;
    int plot_h = h - x_label_padding_bottom;

    if (plot_w <= 20 || plot_h <= 10) { // Check for minimal drawable area
         display.setCursor(x_base, y_base + h / 2);
         display.print("Too small for graph.");
        return; 
    }

    float min_temp = temps[0], max_temp = temps[0];
    float min_wind = winds[0], max_wind = winds[0];

    for (int i = 1; i < num_points; ++i) {
        if (temps[i] < min_temp) min_temp = temps[i];
        if (temps[i] > max_temp) max_temp = temps[i];
        if (winds[i] < min_wind) min_wind = winds[i];
        if (winds[i] > max_wind) max_wind = winds[i];
    }
    if (max_temp == min_temp) max_temp += 1.0f;
    if (max_wind == min_wind) max_wind += 1.0f;

    display.drawRect(plot_x, plot_y, plot_w, plot_h, GxEPD_LIGHTGREY);

    float x_step = (num_points > 1) ? (float)plot_w / (num_points - 1) : 0;

    display.setTextColor(GxEPD_BLACK);
    display.setFont(labelFont);

    // Y-axis labels (Temperature - Left)
    String min_temp_str = String(min_temp, 0);
    String max_temp_str = String(max_temp, 0);
    display.getTextBounds(max_temp_str, 0, 0, &x1_bounds_val, &y1_bounds_val, &w_val, &text_h_val);
    display.setCursor(x_base + y_label_padding_left - w_val -3 , plot_y + text_h_val); 
    display.print(max_temp_str);
    display.getTextBounds(min_temp_str, 0, 0, &x1_bounds_val, &y1_bounds_val, &w_val, &text_h_val);
    display.setCursor(x_base + y_label_padding_left - w_val -3, plot_y + plot_h); 
    display.print(min_temp_str);

    // Y-axis labels (Wind - Right)
    String min_wind_str = String(min_wind, 0);
    String max_wind_str = String(max_wind, 0);
    display.getTextBounds(max_wind_str, 0, 0, &x1_bounds_val, &y1_bounds_val, &w_val, &text_h_val);
    display.setCursor(plot_x + plot_w + 3, plot_y + text_h_val);
    display.print(max_wind_str);
    display.getTextBounds(min_wind_str, 0, 0, &x1_bounds_val, &y1_bounds_val, &w_val, &text_h_val);
    display.setCursor(plot_x + plot_w + 3, plot_y + plot_h);
    display.print(min_wind_str);

    // Plot Temperatures
    for (int i = 0; i < num_points - 1; ++i) {
        int x1 = plot_x + round(i * x_step);
        int y1_temp = plot_y + plot_h - round(((temps[i] - min_temp) / (max_temp - min_temp)) * plot_h);
        int x2 = plot_x + round((i + 1) * x_step);
        int y2_temp = plot_y + plot_h - round(((temps[i+1] - min_temp) / (max_temp - min_temp)) * plot_h);
        if (y1_temp < plot_y) y1_temp = plot_y; if (y1_temp > plot_y + plot_h) y1_temp = plot_y + plot_h; // Clip to bounds
        if (y2_temp < plot_y) y2_temp = plot_y; if (y2_temp > plot_y + plot_h) y2_temp = plot_y + plot_h; // Clip to bounds
        display.drawLine(x1, y1_temp, x2, y2_temp, GxEPD_BLACK);
    }

    // Plot Wind Speeds (now scaled independently)
    for (int i = 0; i < num_points - 1; ++i) {
        int x1 = plot_x + round(i * x_step);
        int y1_wind = plot_y + plot_h - round(((winds[i] - min_wind) / (max_wind - min_wind)) * plot_h);
        int x2 = plot_x + round((i + 1) * x_step);
        int y2_wind = plot_y + plot_h - round(((winds[i+1] - min_wind) / (max_wind - min_wind)) * plot_h);
        if (y1_wind < plot_y) y1_wind = plot_y; if (y1_wind > plot_y + plot_h) y1_wind = plot_y + plot_h; // Clip
        if (y2_wind < plot_y) y2_wind = plot_y; if (y2_wind > plot_y + plot_h) y2_wind = plot_y + plot_h; // Clip
        display.drawLine(x1, y1_wind, x2, y2_wind, GxEPD_DARKGREY); 
    }

    // Draw vertical line and last update time for lastUpdateTime
    String lastUpdateStr = weather.getLastUpdateTime();
    int lastUpdateMinutes = parseHHMMtoMinutes(lastUpdateStr);
    int final_line_x = -1; // Store the calculated x-position for the line

    if (lastUpdateMinutes != -1 && num_points > 0 && times.size() > 0) {
        int firstHourMinutes = parseHHMMtoMinutes(times[0]);
        if (firstHourMinutes != -1 && lastUpdateMinutes >= firstHourMinutes) {
            for (int i = 0; i < num_points; ++i) {
                if (i >= times.size()) break; // Safety break for times access
                int currentPointMinutes = parseHHMMtoMinutes(times[i]);
                if (currentPointMinutes == -1) continue;

                bool found_position = false;
                if (i + 1 < num_points && (i + 1) < times.size()) { // Check if there's a next point to form an interval
                    int nextPointMinutes = parseHHMMtoMinutes(times[i+1]);
                    if (nextPointMinutes == -1) continue;
                    
                    if (currentPointMinutes == nextPointMinutes) { // Avoid division by zero if times are identical for segment
                         if (lastUpdateMinutes == currentPointMinutes) {
                            final_line_x = round(plot_x + i * x_step);
                            found_position = true;
                        }
                    } else if (lastUpdateMinutes >= currentPointMinutes && lastUpdateMinutes < nextPointMinutes) {
                        // Interpolate position within the segment
                        float fraction = (float)(lastUpdateMinutes - currentPointMinutes) / (nextPointMinutes - currentPointMinutes);
                        final_line_x = round((plot_x + i * x_step) + fraction * x_step);
                        found_position = true;
                    }
                } else if (lastUpdateMinutes == currentPointMinutes) { // Exactly at the current (last) point
                     final_line_x = round(plot_x + i * x_step);
                     found_position = true;
                }
                
                if (found_position) {
                    break; // Exit loop once position is found
                }
            }
        }
    }

    if (final_line_x != -1 && final_line_x >= plot_x && final_line_x <= plot_x + plot_w) {
        display.drawFastVLine(final_line_x, plot_y, plot_h, GxEPD_BLACK);

        // Draw the last update time string below the line
        // display.setFont(labelFont); // Font is already set at the beginning of drawMeteogram
        // display.setTextColor(GxEPD_BLACK); // Color is also set

        int16_t x1_time_val, y1_time_val;
        uint16_t w_time_val, h_time_val;
        display.getTextBounds(lastUpdateStr, 0, 0, &x1_time_val, &y1_time_val, &w_time_val, &h_time_val);
        
        // Center the text under the line
        int time_label_x = final_line_x - w_time_val / 2;

        // Constrain x position to be within the meteogram base area
        if (time_label_x < x_base) time_label_x = x_base;
        if (time_label_x + w_time_val > x_base + w) time_label_x = x_base + w - w_time_val;

        // text_h_val was font height, x_label_padding_bottom used it.
        // y position for the text, similar to where former x-axis labels were
        int time_label_y = plot_y + plot_h + x_label_padding_bottom -5; 

        display.setCursor(time_label_x, time_label_y);
        display.print(lastUpdateStr);
    }
}

void goToSleep() {
    Serial.println("Going to deep sleep for " + String(sleepTime / 1000000) + " seconds");
    
    esp_sleep_enable_timer_wakeup(sleepTime);
    esp_deep_sleep_start();
}

void displayWifiErrorIcon() {
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

void setup() {
    Serial.begin(115200);
    
    pinMode(BATTERY_PIN, INPUT);
    SPI.begin(18, 19, 23);
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.hibernate();
    
    wifi.connect();
    if (!wifi.isConnected()) {
        Serial.println("Failed to connect to WiFi");
        displayWifiErrorIcon();
        goToSleep();
        return;
    }
    
    weather.update();
    displayWeather(weather);
    
    goToSleep();
}

void loop() {
    // Most of the work is done in setup() followed by deep sleep
    // The loop() function won't be executed unless something prevents deep sleep
} 