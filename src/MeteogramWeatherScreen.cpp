#include "MeteogramWeatherScreen.h"

#include <algorithm>
#include <vector>

#include "battery.h"

MeteogramWeatherScreen::MeteogramWeatherScreen(DisplayType &display, const WeatherForecast &forecast)
    : display(display),
      forecast(forecast),
      primaryFont(u8g2_font_helvR14_tf),
      secondaryFont(u8g2_font_helvR10_tf),
      smallFont(u8g2_font_micro_tr),
      labelFont(u8g2_font_nokiafc22_tn) {
  gfx.begin(display);
}

int MeteogramWeatherScreen::parseHHMMtoMinutes(const String &hhmm) {
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

void MeteogramWeatherScreen::drawDottedLine(int x0, int y0, int x1, int y1, uint16_t color) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;

  int x = x0;
  int y = y0;
  int dotCount = 0;
  const int dotLength = 2;
  const int gapLength = 2;

  while (true) {
    if (dotCount < dotLength) {
      display.drawPixel(x, y, color);
    }

    dotCount++;
    if (dotCount >= dotLength + gapLength) {
      dotCount = 0;
    }

    if (x == x1 && y == y1) break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x += sx;
    }
    if (e2 < dx) {
      err += dx;
      y += sy;
    }
  }
}

void MeteogramWeatherScreen::render() {
  Serial.println("Displaying meteogram screen");

  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);

  String batteryStatus = getBatteryStatus();
  String windDisplay = String(forecast.currentWindSpeed, 1) + " - " + String(forecast.currentWindGusts, 1) + " m/s";

  gfx.setFontMode(1);
  gfx.setFontDirection(0);
  gfx.setForegroundColor(GxEPD_BLACK);
  gfx.setBackgroundColor(GxEPD_WHITE);
  gfx.setFont(secondaryFont);

  int text_height = gfx.getFontAscent() - gfx.getFontDescent();

  int wind_y = display.height() - 3;
  int temp_y = wind_y - text_height - 8;

  int meteogramX = 0;
  int meteogramY = 2;
  int meteogramW = display.width();
  int meteogramH = temp_y - meteogramY - 15;
  drawMeteogram(meteogramX, meteogramY, meteogramW, meteogramH);

  gfx.setFont(primaryFont);
  gfx.setCursor(6, temp_y);

  String temperatureDisplay = String(forecast.currentTemperature, 1) + " °C";
  gfx.print(temperatureDisplay);

  int temp_width = gfx.getUTF8Width(temperatureDisplay.c_str());

  gfx.setFont(secondaryFont);
  gfx.setCursor(6 + temp_width + 8, temp_y);
  gfx.print(" " + forecast.currentWeatherDescription);

  gfx.setCursor(6, wind_y);
  gfx.print(windDisplay);

  gfx.setForegroundColor(GxEPD_BLACK);
  gfx.setFont(smallFont);
  int battery_width = gfx.getUTF8Width(batteryStatus.c_str());
  int battery_height = gfx.getFontAscent() - gfx.getFontDescent();
  gfx.setCursor(display.width() - battery_width - 2, display.height() - 1);
  gfx.print(batteryStatus);

  display.displayWindow(0, 0, display.width(), display.height());
  display.hibernate();
  Serial.println("Display updated");
}

void MeteogramWeatherScreen::drawMeteogram(int x_base, int y_base, int w, int h) {
  gfx.setFont(smallFont);
  gfx.setFontMode(1);
  gfx.setForegroundColor(GxEPD_BLACK);
  gfx.setBackgroundColor(GxEPD_WHITE);

  if (forecast.hourlyTemperatures.empty() || forecast.hourlyWindSpeeds.empty() || forecast.hourlyWindGusts.empty() ||
      forecast.hourlyTime.empty() || forecast.hourlyPrecipitation.empty() || forecast.hourlyCloudCoverage.empty()) {
    gfx.setCursor(x_base, y_base + h / 2);
    gfx.print("No meteogram data.");
    return;
  }

  int num_points = std::min({(int)forecast.hourlyTemperatures.size(), (int)forecast.hourlyWindSpeeds.size(),
                             (int)forecast.hourlyWindGusts.size(), (int)forecast.hourlyTime.size(),
                             (int)forecast.hourlyPrecipitation.size(), (int)forecast.hourlyCloudCoverage.size(), 24});
  if (num_points <= 1) {
    gfx.setFont(labelFont);
    gfx.setCursor(x_base, y_base + h / 2);
    gfx.print("Not enough data.");
    return;
  }

  float min_temp =
      *std::min_element(forecast.hourlyTemperatures.begin(), forecast.hourlyTemperatures.begin() + num_points);
  float max_temp =
      *std::max_element(forecast.hourlyTemperatures.begin(), forecast.hourlyTemperatures.begin() + num_points);

  std::vector<float> allWindData;
  allWindData.insert(allWindData.end(), forecast.hourlyWindSpeeds.begin(),
                     forecast.hourlyWindSpeeds.begin() + num_points);
  allWindData.insert(allWindData.end(), forecast.hourlyWindGusts.begin(),
                     forecast.hourlyWindGusts.begin() + num_points);
  float min_wind = *std::min_element(allWindData.begin(), allWindData.end());
  float max_wind = *std::max_element(allWindData.begin(), allWindData.end());

  float max_precipitation =
      *std::max_element(forecast.hourlyPrecipitation.begin(), forecast.hourlyPrecipitation.begin() + num_points);

  if (max_temp == min_temp) max_temp += 1.0f;
  if (max_wind == min_wind) max_wind += 1.0f;
  if (max_precipitation == 0.0f) max_precipitation = 1.0f;

  uint16_t label_w, label_h;
  gfx.setFont(labelFont);
  label_w = gfx.getUTF8Width("99");
  label_h = gfx.getFontAscent() - gfx.getFontDescent();

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
    gfx.setFont(labelFont);
    gfx.setCursor(x_base, y_base + h / 2);
    gfx.print("Too small for graph.");
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
    float coverage = forecast.hourlyCloudCoverage[i];
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

  for (int i = 0; i < num_points; ++i) {
    if (forecast.hourlyPrecipitation[i] > 0.0f) {
      int x_center = plot_x + round(i * x_step);
      int bar_width = max(1, (int)(x_step * 0.6f));
      int bar_height = round((forecast.hourlyPrecipitation[i] / max_precipitation) * plot_h);

      int bar_x = x_center - bar_width / 2;
      int bar_y = plot_y + plot_h - bar_height;

      bar_x = constrain(bar_x, plot_x, plot_x + plot_w - bar_width);
      bar_y = constrain(bar_y, plot_y, plot_y + plot_h);
      bar_height = constrain(bar_height, 0, plot_y + plot_h - bar_y);

      display.fillRect(bar_x, bar_y, bar_width, bar_height, GxEPD_DARKGREY);
    }
  }

  gfx.setFont(labelFont);
  gfx.setForegroundColor(GxEPD_BLACK);

  String temp_labels[] = {String(max_temp, 0), String(min_temp, 0)};
  String wind_labels[] = {String(max_wind, 0), String(min_wind, 0)};

  // Proper vertical alignment: top label at top of plot, bottom label at bottom of plot
  int font_ascent = gfx.getFontAscent();
  int font_descent = gfx.getFontDescent();
  int y_positions[] = {plot_y + font_ascent, plot_y + plot_h - font_descent};

  for (int i = 0; i < 2; i++) {
    label_w = gfx.getUTF8Width(temp_labels[i].c_str());
    gfx.setCursor(plot_x - label_w - 3, y_positions[i]);
    gfx.print(temp_labels[i]);

    gfx.setCursor(plot_x + plot_w + 3, y_positions[i]);
    gfx.print(wind_labels[i]);
  }

  for (int i = 0; i < num_points - 1; ++i) {
    int x1 = plot_x + round(i * x_step);
    int x2 = plot_x + round((i + 1) * x_step);

    int y1_temp =
        plot_y + plot_h - round(((forecast.hourlyTemperatures[i] - min_temp) / (max_temp - min_temp)) * plot_h);
    int y2_temp =
        plot_y + plot_h - round(((forecast.hourlyTemperatures[i + 1] - min_temp) / (max_temp - min_temp)) * plot_h);

    int y1_wind = plot_y + plot_h - round(((forecast.hourlyWindSpeeds[i] - min_wind) / (max_wind - min_wind)) * plot_h);
    int y2_wind =
        plot_y + plot_h - round(((forecast.hourlyWindSpeeds[i + 1] - min_wind) / (max_wind - min_wind)) * plot_h);

    int y1_gust = plot_y + plot_h - round(((forecast.hourlyWindGusts[i] - min_wind) / (max_wind - min_wind)) * plot_h);
    int y2_gust =
        plot_y + plot_h - round(((forecast.hourlyWindGusts[i + 1] - min_wind) / (max_wind - min_wind)) * plot_h);

    display.drawLine(constrain(x1, plot_x, plot_x + plot_w - 1), constrain(y1_temp, plot_y, plot_y + plot_h - 1),
                     constrain(x2, plot_x, plot_x + plot_w - 1), constrain(y2_temp, plot_y, plot_y + plot_h - 1),
                     GxEPD_BLACK);
    drawDottedLine(constrain(x1, plot_x, plot_x + plot_w - 1), constrain(y1_wind, plot_y, plot_y + plot_h - 1),
                   constrain(x2, plot_x, plot_x + plot_w - 1), constrain(y2_wind, plot_y, plot_y + plot_h - 1),
                   GxEPD_BLACK);
    drawDottedLine(constrain(x1, plot_x, plot_x + plot_w - 1), constrain(y1_gust, plot_y, plot_y + plot_h - 1),
                   constrain(x2, plot_x, plot_x + plot_w - 1), constrain(y2_gust, plot_y, plot_y + plot_h - 1),
                   GxEPD_BLACK);
  }

  display.drawRect(plot_x, plot_y, plot_w, plot_h, GxEPD_BLACK);

  String lastUpdateStr = forecast.lastUpdateTime;
  int lastUpdateMinutes = parseHHMMtoMinutes(lastUpdateStr);
  int final_line_x = -1;

  if (lastUpdateMinutes != -1 && num_points > 1) {
    for (int i = 0; i < num_points; ++i) {
      int currentMinutes = parseHHMMtoMinutes(forecast.hourlyTime[i]);
      if (currentMinutes == -1) continue;

      if (lastUpdateMinutes == currentMinutes) {
        final_line_x = plot_x + round(i * x_step);
        break;
      }

      if (i < num_points - 1) {
        int nextMinutes = parseHHMMtoMinutes(forecast.hourlyTime[i + 1]);
        if (nextMinutes != -1 && lastUpdateMinutes > currentMinutes && lastUpdateMinutes < nextMinutes) {
          float fraction = (float)(lastUpdateMinutes - currentMinutes) / (nextMinutes - currentMinutes);
          final_line_x = plot_x + round((i + fraction) * x_step);
          break;
        }
      }
    }
  }

  if (final_line_x != -1 && final_line_x >= plot_x && final_line_x <= plot_x + plot_w) {
    display.drawFastVLine(final_line_x, plot_y, plot_h, GxEPD_BLACK);

    gfx.setFont(labelFont);
    label_w = gfx.getUTF8Width(lastUpdateStr.c_str());
    int time_x = constrain(final_line_x - label_w / 2, x_base, x_base + w - label_w);
    gfx.setCursor(time_x, plot_y + plot_h + bottom_padding - 4);
    gfx.print(lastUpdateStr);
  }
}

int MeteogramWeatherScreen::nextRefreshInSeconds() { return 900; }