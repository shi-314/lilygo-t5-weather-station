#ifndef DISPLAY_TYPE_H
#define DISPLAY_TYPE_H

#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>

// Define here which display type to use
#define ARDUINO_LILYGO_T5_V213_4G 1

#if defined(ARDUINO_LILYGO_T5_V213)

using DisplayType = GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>;
using Epd2Type = GxEPD2_213_GDEY0213B74;

#elif defined(ARDUINO_LILYGO_T5_V213_4G)

using DisplayType = GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>;
using Epd2Type = GxEPD2_213_GDEY0213B74;

#endif

#endif