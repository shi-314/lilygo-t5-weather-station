#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <time.h>

#include "ChatGPTClient.h"
#include "ConfigurationScreen.h"
#include "ConfigurationServer.h"
#include "MessageScreen.h"
#include "MeteogramWeatherScreen.h"
#include "OpenMeteoAPI.h"
#include "WiFiConnection.h"
#include "WifiErrorScreen.h"
#include "battery.h"
#include "boards.h"

RTC_DATA_ATTR char wifiSSID[64] = "";
RTC_DATA_ATTR char wifiPassword[64] = "";
RTC_DATA_ATTR char openaiApiKey[200] = "";
RTC_DATA_ATTR char aiPromptStyle[200] = "neutral";

const String aiWeatherPrompt =
    "I will share a JSON payload with you from the Open Meteo API which has weather forecast data for the current "
    "day. You have to summarize it into one sentence:\n"
    "- 18 words or less\n"
    "- include the rough temperature in the sentence\n"
    "- forecast for the whole day is included in the sentence\n"
    "- use the time of the day to make the sentence more interesting, but don't mention the exact time\n"
    "- don't mention the location\n"
    "- only include the current weather and the forecast for the remaining day, not the past\n";

// Berlin
const float latitude = 52.520008;
const float longitude = 13.404954;

GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(
    GxEPD2_213_GDEY0213B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

const unsigned long deepSleepMicros = 900000000;  // Deep sleep time in microseconds (15 minutes)

OpenMeteoAPI openMeteoAPI;
ConfigurationServer configurationServer(Configuration(wifiSSID, wifiPassword, openaiApiKey, aiPromptStyle));

enum ScreenType { CONFIG_SCREEN = 0, METEOGRAM_SCREEN = 1, MESSAGE_SCREEN = 2, SCREEN_COUNT = 3 };

RTC_DATA_ATTR int currentScreenIndex = METEOGRAM_SCREEN;

void goToSleep(uint64_t sleepTime);
void displayCurrentScreen();
void cycleToNextScreen();
bool isButtonWakeup();
void updateConfiguration(const Configuration& config);
bool hasValidOpenaiApiKey();

bool hasValidWiFiCredentials() { return strlen(wifiSSID) > 0 && strlen(wifiPassword) > 0; }

bool hasValidOpenaiApiKey() { return strlen(openaiApiKey) > 0; }

bool isButtonWakeup() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  return (wakeupReason == ESP_SLEEP_WAKEUP_EXT0);
}

void cycleToNextScreen() {
  currentScreenIndex = (currentScreenIndex + 1) % SCREEN_COUNT;

  // Skip MESSAGE_SCREEN if no OpenAI API key is configured
  if (currentScreenIndex == MESSAGE_SCREEN && !hasValidOpenaiApiKey()) {
    currentScreenIndex = (currentScreenIndex + 1) % SCREEN_COUNT;
  }

  Serial.println("Cycled to screen: " + String(currentScreenIndex));
}

void displayCurrentScreen() {
  switch (currentScreenIndex) {
    case CONFIG_SCREEN: {
      ConfigurationScreen configurationScreen(display, configurationServer.getWifiAccessPointName(),
                                              configurationServer.getWifiAccessPointPassword());
      configurationScreen.render();

      configurationServer.run(updateConfiguration);

      while (configurationServer.isRunning()) {
        configurationServer.handleRequests();

        if (digitalRead(BUTTON_1) == LOW) {
          delay(50);
          if (digitalRead(BUTTON_1) == LOW) {
            Serial.println("Button pressed - exiting configuration mode");
            break;
          }
        }

        delay(10);
      }

      configurationServer.stop();
      break;
    }
    case METEOGRAM_SCREEN: {
      WeatherForecast forecastData = openMeteoAPI.getForecast(latitude, longitude);
      MeteogramWeatherScreen meteogramWeatherScreen(display, forecastData);
      meteogramWeatherScreen.render();
      break;
    }
    case MESSAGE_SCREEN: {
      WeatherForecast forecastData = openMeteoAPI.getForecast(latitude, longitude);

      String prompt = aiWeatherPrompt;
      prompt += "- Use the following style: " + String(aiPromptStyle) + "\n";
      prompt += forecastData.apiPayload;

      ChatGPTClient chatGPTClient(openaiApiKey);
      String chatGPTResponse = chatGPTClient.generateContent(prompt);

      MessageScreen messageScreen(display);
      messageScreen.setMessageText(chatGPTResponse);
      messageScreen.render();
      break;
    }
    default: {
      Serial.println("Unknown screen index, defaulting to meteogram");
      currentScreenIndex = METEOGRAM_SCREEN;
      WeatherForecast forecastData = openMeteoAPI.getForecast(latitude, longitude);
      MeteogramWeatherScreen meteogramWeatherScreen(display, forecastData);
      meteogramWeatherScreen.render();
      break;
    }
  }
}

void updateConfiguration(const Configuration& config) {
  if (config.ssid.length() >= sizeof(wifiSSID)) {
    Serial.println("Error: SSID too long, maximum length is " + String(sizeof(wifiSSID) - 1));
    return;
  }

  if (config.password.length() >= sizeof(wifiPassword)) {
    Serial.println("Error: Password too long, maximum length is " + String(sizeof(wifiPassword) - 1));
    return;
  }

  if (config.openaiApiKey.length() >= sizeof(openaiApiKey)) {
    Serial.println("Error: OpenAI API key too long, maximum length is " + String(sizeof(openaiApiKey) - 1));
    return;
  }

  if (config.aiPromptStyle.length() >= sizeof(aiPromptStyle)) {
    Serial.println("Error: AI Prompt Style too long, maximum length is " + String(sizeof(aiPromptStyle) - 1));
    return;
  }

  memset(wifiSSID, 0, sizeof(wifiSSID));
  memset(wifiPassword, 0, sizeof(wifiPassword));
  memset(openaiApiKey, 0, sizeof(openaiApiKey));
  memset(aiPromptStyle, 0, sizeof(aiPromptStyle));

  strncpy(wifiSSID, config.ssid.c_str(), sizeof(wifiSSID) - 1);
  strncpy(wifiPassword, config.password.c_str(), sizeof(wifiPassword) - 1);
  strncpy(openaiApiKey, config.openaiApiKey.c_str(), sizeof(openaiApiKey) - 1);
  strncpy(aiPromptStyle, config.aiPromptStyle.c_str(), sizeof(aiPromptStyle) - 1);

  Serial.println("Configuration updated in RTC memory");
  Serial.println("WiFi SSID: " + String(wifiSSID));
  Serial.println("OpenAI API Key: " + String(hasValidOpenaiApiKey() ? "[CONFIGURED]" : "[NOT SET]"));
  Serial.println("AI Prompt Style: " + String(strlen(aiPromptStyle) > 0 ? aiPromptStyle : "[NOT SET]"));
}

void goToSleep(uint64_t sleepTime) {
  Serial.println("Going to deep sleep for " + String(sleepTime / 1000000) + " seconds");
  Serial.println("Press button to wake up early and cycle screens");

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
  esp_sleep_enable_timer_wakeup(sleepTime);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  pinMode(BATTERY_PIN, INPUT);
  pinMode(BUTTON_1, INPUT_PULLUP);
  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);

  if (!isButtonWakeup()) {
    if (!hasValidWiFiCredentials()) {
      currentScreenIndex = CONFIG_SCREEN;
    } else {
      currentScreenIndex = METEOGRAM_SCREEN;
    }
  }

  if (isButtonWakeup()) {
    cycleToNextScreen();
  }

  if (currentScreenIndex != CONFIG_SCREEN) {
    WiFiConnection wifi(wifiSSID, wifiPassword);
    wifi.connect();
    if (!wifi.isConnected()) {
      Serial.println("Failed to connect to WiFi");
      WifiErrorScreen errorScreen(display);
      errorScreen.render();
      goToSleep(deepSleepMicros);
      return;
    }
  }

  displayCurrentScreen();

  if (currentScreenIndex == CONFIG_SCREEN) {
    goToSleep(1000000);  // 1 second
  } else {
    goToSleep(deepSleepMicros);
  }
}

void loop() {}