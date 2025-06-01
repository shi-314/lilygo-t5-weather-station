# LilyGO T5 Weather Station

A weather station for LilyGO T5 e-paper displays that shows current weather, forecasts, and AI-generated weather summaries.

## Features

- Current weather display
- Meteorogram/forecast view
- AI-powered weather summaries (requires OpenAI API key)
- Configuration via web interface
- Deep sleep mode for battery efficiency

## Hardware Required

- [LilyGO T5 v2.13 e-paper display board](https://lilygo.cc/en-pl/products/t5-v2-3-1?variant=42366871666869)
- [Case](https://www.thingiverse.com/thing:4670205/files)
- LiPo battery: I used a 3.7V 500mAh LiPo battery
- WiFi connection
- Mobile phone to connect to the device and configure it

## Setup

1. Install PlatformIO
2. Clone this repository
3. Build and upload:
   ```bash
   pio run --target upload
   ```

## Configuration

1. Power on the device (shows configuration screen automatically on first boot)
2. Connect to the WiFi hotspot created by the device
3. Your web browser will open automatically showing the configuration page
4. Configure WiFi credentials, location, and optionally OpenAI API key

To re-enter configuration mode later, press the button while the device is running.

## Usage

- **Button press**: Cycle through screens or enter configuration mode.
- **Screens**: Configuration → Current weather → Meteogram → AI summary (if configured).
- **Auto-refresh**: Updates every 15 minutes and goes back into deep sleep mode.