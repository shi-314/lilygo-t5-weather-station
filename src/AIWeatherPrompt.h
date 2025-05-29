#ifndef AI_WEATHER_PROMPT_H
#define AI_WEATHER_PROMPT_H

#include <Arduino.h>
#include "Weather.h"

class AIWeatherPrompt {
public:
    AIWeatherPrompt();
    
    String generatePrompt(const Weather& weather);
};

#endif 