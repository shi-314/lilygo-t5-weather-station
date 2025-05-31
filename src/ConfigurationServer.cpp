#include "ConfigurationServer.h"
#include <WiFi.h>
#include <WiFiAP.h>

ConfigurationServer::ConfigurationServer()
    : deviceName("LilyGo-Weather-Station"),
      wifiAccessPointName("WeatherStation-Config"),
      wifiAccessPointPassword("configure123"),
      server(nullptr),
      dnsServer(nullptr),
      isRunning(false)
{
}

void ConfigurationServer::run()
{
    Serial.println("Starting Configuration Server...");
    Serial.print("Device Name: ");
    Serial.println(deviceName);

    WiFi.disconnect(true);
    delay(1000);

    Serial.print("Setting up WiFi Access Point: ");
    Serial.println(wifiAccessPointName);

    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(wifiAccessPointName.c_str(), wifiAccessPointPassword.c_str());

    if (apStarted)
    {
        Serial.println("Access Point started successfully!");
        Serial.print("Network Name (SSID): ");
        Serial.println(wifiAccessPointName);
        Serial.print("Password: ");
        Serial.println(wifiAccessPointPassword);
        Serial.print("Access Point IP: ");
        Serial.println(WiFi.softAPIP());
        Serial.println("Setting up captive portal...");

        setupDNSServer();
        setupWebServer();

        isRunning = true;
        Serial.println("Captive portal is running!");
        Serial.println("Devices connecting to this network will be automatically redirected to the configuration page");
    }
    else
    {
        Serial.println("Failed to start Access Point!");
    }
}

void ConfigurationServer::stop()
{
    if (isRunning)
    {
        if (server)
        {
            delete server;
            server = nullptr;
        }
        if (dnsServer)
        {
            dnsServer->stop();
            delete dnsServer;
            dnsServer = nullptr;
        }
        WiFi.softAPdisconnect(true);
        isRunning = false;
        Serial.println("Configuration server stopped");
    }
}

void ConfigurationServer::handleRequests()
{
    if (isRunning && dnsServer)
    {
        dnsServer->processNextRequest();
    }
}

void ConfigurationServer::setupDNSServer()
{
    dnsServer = new DNSServer();
    const byte DNS_PORT = 53;
    dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.println("DNS Server started - all domains redirect to captive portal");
}

void ConfigurationServer::setupWebServer()
{
    server = new AsyncWebServer(80);

    server->on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); }); // Android
    server->on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); }); // Microsoft
    server->on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); }); // iOS
    server->on("/connectivity-check.html", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); }); // Firefox

    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); });
    server->on("/config", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); });
    server->on("/save", HTTP_POST, [this](AsyncWebServerRequest *request)
               { handleSave(request); });

    server->onNotFound([this](AsyncWebServerRequest *request)
                       { handleNotFound(request); });

    server->begin();
    Serial.println("Web server started on port 80");
}

void ConfigurationServer::handleRoot(AsyncWebServerRequest *request)
{
    String html = getConfigurationPage();
    request->send(200, "text/html", html);
}

void ConfigurationServer::handleSave(AsyncWebServerRequest *request)
{
    String response = "<html><body><h2>Configuration Saved!</h2>";
    response += "<p>Settings have been saved. The device will restart in 3 seconds.</p>";
    response += "<script>setTimeout(function(){ window.close(); }, 3000);</script>";
    response += "</body></html>";

    if (request->hasParam("ssid", true) && request->hasParam("password", true))
    {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();

        Serial.println("WiFi Configuration received:");
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println("***");

        response += "<p>SSID: " + ssid + "</p>";
        response += "<p>The device will now connect to your WiFi network.</p>";
    }

    request->send(200, "text/html", response);
}

void ConfigurationServer::handleNotFound(AsyncWebServerRequest *request)
{
    request->redirect("/");
}

String ConfigurationServer::getConfigurationPage()
{
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weather Station Configuration</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 500px;
            margin: 20px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
            color: #555;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 16px;
            box-sizing: border-box;
        }
        input[type="text"]:focus, input[type="password"]:focus {
            border-color: #4CAF50;
            outline: none;
        }
        .btn {
            background-color: #4CAF50;
            color: white;
            padding: 12px 30px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            width: 100%;
        }
        .btn:hover {
            background-color: #45a049;
        }
        .info {
            background-color: #e7f3ff;
            border: 1px solid #b3d9ff;
            padding: 15px;
            border-radius: 5px;
            margin-bottom: 20px;
        }
        .device-info {
            text-align: center;
            color: #666;
            margin-bottom: 20px;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌤️ Weather Station Setup</h1>
        
        <div class="device-info">
            <strong>Device:</strong> )" +
                  deviceName + R"(<br>
            <strong>Network:</strong> )" +
                  wifiAccessPointName + R"(
        </div>
        
        <div class="info">
            <strong>Welcome!</strong> Configure your weather station to connect to your WiFi network.
        </div>
        
        <form method="POST" action="/save">
            <div class="form-group">
                <label for="ssid">WiFi Network Name (SSID):</label>
                <input type="text" id="ssid" name="ssid" required placeholder="Enter your WiFi network name">
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password:</label>
                <input type="password" id="password" name="password" required placeholder="Enter your WiFi password">
            </div>
            
            <button type="submit" class="btn">Save Configuration</button>
        </form>
    </div>
</body>
</html>
    )";

    return html;
}

String ConfigurationServer::getWifiAccessPointName() const
{
    return wifiAccessPointName;
}

String ConfigurationServer::getWifiAccessPointPassword() const
{
    return wifiAccessPointPassword;
}