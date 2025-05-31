# Data Directory - SPIFFS Files

This directory contains files that will be uploaded to the ESP32's SPIFFS (SPI Flash File System).

## Files

- `config.html` - Configuration page template for the WiFi setup captive portal

## Template Variables

The `config.html` file uses simple placeholder replacement:
- `{{DEVICE_NAME}}` - Replaced with the device name
- `{{AP_NAME}}` - Replaced with the WiFi access point name

## Uploading Files

To upload the files in this directory to the ESP32:

1. **PlatformIO CLI:**
   ```bash
   pio run --target uploadfs
   ```

2. **PlatformIO IDE:**
   - Open PlatformIO tab
   - Go to your project
   - Under "Platform" section, click "Upload Filesystem Image"

## Editing the UI

You can now edit `config.html` directly with:
- Full HTML syntax highlighting
- CSS intellisense
- Easy file management
- Better formatting tools

After making changes, just upload the filesystem image again to update the ESP32. 