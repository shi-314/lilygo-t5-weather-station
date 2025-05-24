#include "Rendering.h"
#include "battery.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include "Fonts/Picopixel.h"
#include "assets/WifiErrorIcon.h"
#include <vector>
#include <algorithm>

Rendering::Rendering(GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>& display) 
    : display(display) {
}

int Rendering::parseHHMMtoMinutes(const String& hhmm) {
    if (hhmm.length() != 5 || hhmm.charAt(2) != ':') {
        return -1;
    }
    int hours = hhmm.substring(0, 2).toInt();
    int minutes = hhmm.substring(3, 5).toInt();
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
        return -1;
    }
    return hours * 60 + minutes;
}

void Rendering::displayWeather(Weather& weather) {
    Serial.println("Updating display...");
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);

    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans12pt7b);

    display.setFont(&FreeSans9pt7b);
    String batteryStatus = getBatteryStatus();
    String windDisplay = String(weather.getCurrentWindSpeed(), 1) + " - " + String(weather.getCurrentWindGusts(), 1) + " m/s";
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds("Temp", 0, 0, &x1, &y1, &w, &h);
    int text_height = h;
    
    int wind_y = display.height() - 3;
    int temp_y = wind_y - text_height - 8;
    
    int meteogramX = 0;
    int meteogramY = 10;
    int meteogramW = display.width();
    int meteogramH = temp_y - meteogramY - 25;
    drawMeteogram(weather, meteogramX, meteogramY, meteogramW, meteogramH);
    
    display.setFont(&FreeSans12pt7b);
    display.setCursor(6, temp_y);
    
    String temperatureDisplay = String(weather.getCurrentTemperature(), 1) + " C";
    display.print(temperatureDisplay);
    
    display.getTextBounds(temperatureDisplay, 0, 0, &x1, &y1, &w, &h);
    
    display.setFont(&FreeSans9pt7b);
    display.setCursor(6 + w + 8, temp_y);
    display.print(" " + weather.getWeatherDescription());
    
    display.setCursor(6, wind_y);
    display.print(windDisplay);
    
    display.setTextColor(GxEPD_DARKGREY);
    display.setFont(nullptr);
    int16_t x1_batt, y1_batt;
    uint16_t w_batt, h_batt;
    display.getTextBounds(batteryStatus, 0, 0, &x1_batt, &y1_batt, &w_batt, &h_batt);
    display.setCursor(display.width() - w_batt - 2, display.height() - h_batt - 1);
    display.print(batteryStatus);
    
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    Serial.println("Display updated");
}

void Rendering::drawMeteogram(Weather& weather, int x_base, int y_base, int w, int h) {
    display.setFont(nullptr);
    std::vector<float> temps = weather.getHourlyTemperatures();
    std::vector<float> winds = weather.getHourlyWindSpeeds();
    std::vector<String> times = weather.getHourlyTime();
    std::vector<float> precipitation = weather.getHourlyPrecipitation();
    std::vector<float> cloudCoverage = weather.getHourlyCloudCoverage();

    if (temps.empty() || winds.empty() || times.empty() || precipitation.empty() || cloudCoverage.empty()) {
        display.setCursor(x_base, y_base + h / 2);
        display.print("No meteogram data.");
        return;
    }

    int num_points = std::min({(int)temps.size(), (int)winds.size(), (int)times.size(), (int)precipitation.size(), (int)cloudCoverage.size(), 24});
    if (num_points <= 1) { 
        display.setCursor(x_base, y_base + h / 2);
        display.print("Not enough data.");
        return;
    }

    float min_temp = *std::min_element(temps.begin(), temps.begin() + num_points);
    float max_temp = *std::max_element(temps.begin(), temps.begin() + num_points);
    float min_wind = *std::min_element(winds.begin(), winds.begin() + num_points);
    float max_wind = *std::max_element(winds.begin(), winds.begin() + num_points);
    float max_precipitation = *std::max_element(precipitation.begin(), precipitation.begin() + num_points);
    
    if (max_temp == min_temp) max_temp += 1.0f;
    if (max_wind == min_wind) max_wind += 1.0f;
    if (max_precipitation == 0.0f) max_precipitation = 1.0f;

    int16_t x1, y1;
    uint16_t label_w, label_h;
    display.getTextBounds("99", 0, 0, &x1, &y1, &label_w, &label_h);
    
    int left_padding = label_w + 6;
    int right_padding = label_w + 6;
    int bottom_padding = label_h + 6;
    
    const int cloud_bar_height = 6;
    const int cloud_bar_spacing = 2;
    
    int plot_x = x_base + left_padding;
    int plot_y = y_base + cloud_bar_height + cloud_bar_spacing;
    int plot_w = w - left_padding - right_padding;
    int plot_h = h - bottom_padding - cloud_bar_height - cloud_bar_spacing;

    if (plot_w <= 20 || plot_h <= 10) {
        display.setCursor(x_base, y_base + h / 2);
        display.print("Too small for graph.");
        return; 
    }

    // Draw cloud coverage bar on top
    float x_step = (float)plot_w / (num_points - 1);
    for (int i = 0; i < num_points - 1; ++i) {
        int x1_pos = plot_x + round(i * x_step);
        int x2_pos = plot_x + round((i + 1) * x_step);
        int segment_width = x2_pos - x1_pos;
        
        // Get cloud coverage color based on percentage
        uint16_t cloudColor;
        float coverage = cloudCoverage[i];
        if (coverage < 25.0f) {
            cloudColor = GxEPD_WHITE;
        } else if (coverage < 50.0f) {
            cloudColor = GxEPD_LIGHTGREY;
        } else if (coverage < 75.0f) {
            cloudColor = GxEPD_DARKGREY;
        } else {
            cloudColor = GxEPD_BLACK;
        }
        
        display.fillRect(x1_pos, y_base, segment_width, cloud_bar_height, cloudColor);
    }
    
    // Draw border around cloud coverage bar
    display.drawRect(plot_x, y_base, plot_w, cloud_bar_height, GxEPD_BLACK);

    display.drawRect(plot_x, plot_y, plot_w, plot_h, GxEPD_LIGHTGREY);
    
    for (int i = 0; i < num_points; ++i) {
        if (precipitation[i] > 0.0f) {
            int x_center = plot_x + round(i * x_step);
            int bar_width = max(1, (int)(x_step * 0.6f));
            int bar_height = round((precipitation[i] / max_precipitation) * plot_h);
            
            int bar_x = x_center - bar_width / 2;
            int bar_y = plot_y + plot_h - bar_height;
            
            bar_x = constrain(bar_x, plot_x, plot_x + plot_w - bar_width);
            bar_y = constrain(bar_y, plot_y, plot_y + plot_h);
            bar_height = constrain(bar_height, 0, plot_y + plot_h - bar_y);
            
            display.fillRect(bar_x, bar_y, bar_width, bar_height, GxEPD_LIGHTGREY);
        }
    }
    
    display.setTextColor(GxEPD_BLACK);

    String temp_labels[] = {String(max_temp, 0), String(min_temp, 0)};
    String wind_labels[] = {String(max_wind, 0), String(min_wind, 0)};
    int y_positions[] = {plot_y, plot_y + plot_h};
    
    for (int i = 0; i < 2; i++) {
        display.getTextBounds(temp_labels[i], 0, 0, &x1, &y1, &label_w, &label_h);
        display.setCursor(plot_x - label_w - 3, y_positions[i]);
        display.print(temp_labels[i]);
        
        display.setCursor(plot_x + plot_w + 3, y_positions[i]);
        display.print(wind_labels[i]);
    }

    for (int i = 0; i < num_points - 1; ++i) {
        int x1 = plot_x + round(i * x_step);
        int x2 = plot_x + round((i + 1) * x_step);
        
        int y1_temp = plot_y + plot_h - round(((temps[i] - min_temp) / (max_temp - min_temp)) * plot_h);
        int y2_temp = plot_y + plot_h - round(((temps[i+1] - min_temp) / (max_temp - min_temp)) * plot_h);
        
        int y1_wind = plot_y + plot_h - round(((winds[i] - min_wind) / (max_wind - min_wind)) * plot_h);
        int y2_wind = plot_y + plot_h - round(((winds[i+1] - min_wind) / (max_wind - min_wind)) * plot_h);
        
        display.drawLine(x1, constrain(y1_temp, plot_y, plot_y + plot_h), 
                        x2, constrain(y2_temp, plot_y, plot_y + plot_h), GxEPD_BLACK);
        display.drawLine(x1, constrain(y1_wind, plot_y, plot_y + plot_h), 
                        x2, constrain(y2_wind, plot_y, plot_y + plot_h), GxEPD_DARKGREY);
    }

    String lastUpdateStr = weather.getLastUpdateTime();
    int lastUpdateMinutes = parseHHMMtoMinutes(lastUpdateStr);
    int final_line_x = -1;

    if (lastUpdateMinutes != -1 && num_points > 1) {
        for (int i = 0; i < num_points; ++i) {
            int currentMinutes = parseHHMMtoMinutes(times[i]);
            if (currentMinutes == -1) continue;
            
            if (lastUpdateMinutes == currentMinutes) {
                final_line_x = plot_x + round(i * x_step);
                break;
            }
            
            if (i < num_points - 1) {
                int nextMinutes = parseHHMMtoMinutes(times[i + 1]);
                if (nextMinutes != -1 && 
                    lastUpdateMinutes > currentMinutes && 
                    lastUpdateMinutes < nextMinutes) {
                    float fraction = (float)(lastUpdateMinutes - currentMinutes) / (nextMinutes - currentMinutes);
                    final_line_x = plot_x + round((i + fraction) * x_step);
                    break;
                }
            }
        }
    }

    if (final_line_x != -1 && final_line_x >= plot_x && final_line_x <= plot_x + plot_w) {
        display.drawFastVLine(final_line_x, plot_y, plot_h, GxEPD_BLACK);

        display.getTextBounds(lastUpdateStr, 0, 0, &x1, &y1, &label_w, &label_h);
        int time_x = constrain(final_line_x - label_w / 2, x_base, x_base + w - label_w);
        display.setCursor(time_x, plot_y + plot_h + bottom_padding - 6);
        display.print(lastUpdateStr);
    }
}

void Rendering::displayWifiErrorIcon() {
    Serial.println("Displaying WiFi error icon");
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    
    int iconWidth = 16;
    int iconHeight = 16;
    
    int centerX = (display.width() / 2) - (iconWidth / 2);
    int centerY = (display.height() / 2) - (iconHeight / 2);
    
    display.drawBitmap(centerX, centerY, WifiErrorIcon, iconWidth, iconHeight, GxEPD_BLACK);
    
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
}