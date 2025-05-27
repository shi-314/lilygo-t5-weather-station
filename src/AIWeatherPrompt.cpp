#include "AIWeatherPrompt.h"

AIWeatherPrompt::AIWeatherPrompt() {
}

String AIWeatherPrompt::generatePrompt(const Weather& weather) {
    String prompt = "I will share a JSON payload with you from the Open Meteo API which has weather forecast data for the current day. ";
    prompt += "You have to summarize it into one sentence in the following style:\n";
    prompt += "- a bizarre conspiracy theory describing the weather\n";
    prompt += "- a funny way\n";
    prompt += "- a short sentence\n";
    prompt += "- include the actual temperature in the sentence\n";
    prompt += "- use simple words\n";
    prompt += "- make sure the weather forecast is included in the sentence\n";
    prompt += "Weather data: ";
    prompt += weather.getLastPayload();
    
    return prompt;
} 