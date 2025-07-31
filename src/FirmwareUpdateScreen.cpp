#include "FirmwareUpdateScreen.h"
#include "battery.h"

FirmwareUpdateScreen::FirmwareUpdateScreen(DisplayType& display)
    : display(display), 
      primaryFont(u8g2_font_helvR12_tr),
      mediumFont(u8g2_font_helvR10_tf), 
      smallFont(u8g2_font_helvR08_tr) {
  gfx.begin(display);
  firmwareInfo.currentVersion = currentFirmwareVersion;
  firmwareInfo.updateAvailable = false;
}

bool FirmwareUpdateScreen::checkForUpdates() {
  Serial.println("Checking for firmware updates...");
  
  HTTPClient http;
  http.begin(githubApiUrl);
  http.addHeader("User-Agent", "LILYGO-T5-Weather-Station");
  
  int httpCode = http.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("GitHub API request failed: %d\n", httpCode);
    http.end();
    return false;
  }
  
  String payload = http.getString();
  http.end();
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return false;
  }
  
  firmwareInfo.latestVersion = doc["tag_name"].as<String>();
  firmwareInfo.updateUrl = doc["html_url"].as<String>();
  firmwareInfo.releaseNotes = doc["body"].as<String>();
  
  // Remove 'v' prefix if present
  String latestClean = firmwareInfo.latestVersion;
  if (latestClean.startsWith("v")) {
    latestClean = latestClean.substring(1);
  }
  
  String currentClean = firmwareInfo.currentVersion;
  if (currentClean.startsWith("v")) {
    currentClean = currentClean.substring(1);
  }
  
  firmwareInfo.updateAvailable = compareVersions(currentClean, latestClean);
  
  Serial.printf("Current: %s, Latest: %s, Update Available: %s\n", 
                currentClean.c_str(), latestClean.c_str(), 
                firmwareInfo.updateAvailable ? "Yes" : "No");
  
  return true;
}

bool FirmwareUpdateScreen::compareVersions(const String& current, const String& latest) {
  // Simple version comparison (assumes semantic versioning: x.y.z)
  int currentMajor = 0, currentMinor = 0, currentPatch = 0;
  int latestMajor = 0, latestMinor = 0, latestPatch = 0;
  
  sscanf(current.c_str(), "%d.%d.%d", &currentMajor, &currentMinor, &currentPatch);
  sscanf(latest.c_str(), "%d.%d.%d", &latestMajor, &latestMinor, &latestPatch);
  
  if (latestMajor > currentMajor) return true;
  if (latestMajor < currentMajor) return false;
  
  if (latestMinor > currentMinor) return true;
  if (latestMinor < currentMinor) return false;
  
  return latestPatch > currentPatch;
}

void FirmwareUpdateScreen::render() {
  Serial.println("Displaying firmware update screen");
  
  display.init(115200);
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  
  gfx.setFontMode(1);  
  gfx.setFontDirection(0);
  gfx.setForegroundColor(GxEPD_BLACK);
  gfx.setBackgroundColor(GxEPD_WHITE);
  
  int displayWidth = display.width();
  int displayHeight = display.height();
  int margin = 8;
  int yPos = margin + 15;
  
  // Title
  gfx.setFont(primaryFont);
  String title = "Firmware Update Check";
  int titleWidth = gfx.getUTF8Width(title.c_str());
  gfx.setCursor((displayWidth - titleWidth) / 2, yPos);
  gfx.print(title);
  yPos += 18;
  
  // Check for updates
  bool updateCheckSuccess = checkForUpdates();
  
  gfx.setFont(mediumFont);
  
  if (!updateCheckSuccess) {
    // Error occurred
    String errorMsg = "Error checking for updates";
    int errorWidth = gfx.getUTF8Width(errorMsg.c_str());
    gfx.setCursor((displayWidth - errorWidth) / 2, yPos);
    gfx.print(errorMsg);
    yPos += 15;
    
    String retryMsg = "Check internet connection";
    int retryWidth = gfx.getUTF8Width(retryMsg.c_str());
    gfx.setCursor((displayWidth - retryWidth) / 2, yPos);
    gfx.print(retryMsg);
  } else {
    // Current version
    String currentText = "Current: " + firmwareInfo.currentVersion;
    int currentWidth = gfx.getUTF8Width(currentText.c_str());
    gfx.setCursor((displayWidth - currentWidth) / 2, yPos);
    gfx.print(currentText);
    yPos += 15;
    
    // Latest version
    String latestText = "Latest: " + firmwareInfo.latestVersion;
    int latestWidth = gfx.getUTF8Width(latestText.c_str());
    gfx.setCursor((displayWidth - latestWidth) / 2, yPos);
    gfx.print(latestText);
    yPos += 18;
    
    // Status
    String statusText;
    if (firmwareInfo.updateAvailable) {
      statusText = "UPDATE AVAILABLE!";
    } else {
      statusText = "Up to date";
    }
    
    int statusWidth = gfx.getUTF8Width(statusText.c_str());
    gfx.setCursor((displayWidth - statusWidth) / 2, yPos);
    gfx.print(statusText);
    yPos += 18;
    
    // Additional info for updates
    if (firmwareInfo.updateAvailable) {
      gfx.setFont(smallFont);
      String infoText = "Visit GitHub releases page";
      int infoWidth = gfx.getUTF8Width(infoText.c_str());
      gfx.setCursor((displayWidth - infoWidth) / 2, yPos);
      gfx.print(infoText);
      yPos += 10;
      
      String urlText = "for download instructions";
      int urlWidth = gfx.getUTF8Width(urlText.c_str());
      gfx.setCursor((displayWidth - urlWidth) / 2, yPos);
      gfx.print(urlText);
    }
  }
  
  // Display battery status in corner
  String batteryStatus = getBatteryStatus();
  gfx.setFont(smallFont);
  gfx.setForegroundColor(GxEPD_BLACK);
  
  int batteryWidth = gfx.getUTF8Width(batteryStatus.c_str());
  gfx.setCursor(displayWidth - batteryWidth - 3, displayHeight - 3);
  gfx.print(batteryStatus);
  
  display.displayWindow(0, 0, displayWidth, displayHeight);
  display.hibernate();
  Serial.println("Firmware update screen displayed");
}

int FirmwareUpdateScreen::nextRefreshInSeconds() {
  // Check for updates every 6 hours (21600 seconds)
  return 21600;
}
