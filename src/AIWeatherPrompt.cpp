#include "AIWeatherPrompt.h"

AIWeatherPrompt::AIWeatherPrompt() {
}

String AIWeatherPrompt::generatePrompt(const Weather& weather) {
    String prompt = "I will share a JSON payload with you from the Open Meteo API which has weather forecast data for the current day. ";
    prompt += "You have to summarize it into one sentence in the following style:\n";
    // prompt += "- a bizarre conspiracy theory describing the weather\n";
    // prompt += "- make sure the sentence is very hilarious\n";
    prompt += "- a short sentence that is 18 words or less\n";
    prompt += "- include the rough temperature in the sentence\n";
    prompt += "- make sure the weather forecast is included in the sentence\n";
    // prompt += "- consider using recent conspiracy theories to make the sentence more interesting\n";
    // prompt += "- the conspiracy theory should be about the weather and somewhat plausible\n";
    prompt += "- you can the location and culture but don't mention the location\n";
    prompt += "- you can use the time of the day to make the sentence more interesting, but don't mention the exact time\n";
    // prompt += "- don't hesitate to be inappropriate\n";
    prompt += "Weather data: ";
    prompt += weather.getLastPayload();
    
    return prompt;
} 