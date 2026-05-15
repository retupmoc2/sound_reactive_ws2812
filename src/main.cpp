#include <Arduino.h>
#include <FastLED.h>
#include "AppConfig.h"
#include "SoundLevelMeter.h"

CRGB leds[LED_COUNT];

SoundLevelMeter soundMeter;

void showVolumeBar(float level)
{
    static float displayedLitCount = 0.0f;

    const float targetLitCount = constrain(level, 0.0f, 1.0f) * LED_COUNT;

    const float attack = 0.35f;   // rise speed
    const float release = 0.08f;  // fall speed

    if (targetLitCount > displayedLitCount)
    {
        displayedLitCount += (targetLitCount - displayedLitCount) * attack;
    }
    else
    {
        displayedLitCount += (targetLitCount - displayedLitCount) * release;
    }

    const uint16_t litCount = min<uint16_t>(
        static_cast<uint16_t>(floorf(displayedLitCount)),
        LED_COUNT);

    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
        if (i >= litCount)
        {
            leds[i] = CRGB::Black;
        }
        else if ((i * 3) < LED_COUNT)
        {
            leds[i] = CRGB::Green;
        }
        else if ((i * 3) < (LED_COUNT * 2))
        {
            leds[i] = CRGB::Yellow;
        }
        else
        {
            leds[i] = CRGB::Red;
        }
    }

    FastLED.show();
}

void printDebug(uint16_t peakToPeak, float noiseFloor, float signalMax, float normalizedLevel)
{
    static uint32_t lastPrintMs = 0;
    const uint32_t nowMs = millis();

    if ((nowMs - lastPrintMs) < DEBUG_PRINT_MS)
    {
        return;
    }

    lastPrintMs = nowMs;

    const float signal = max(0.0f, static_cast<float>(peakToPeak) - noiseFloor - NOISE_GATE_ABOVE_FLOOR);

    Serial.print("peakToPeak=");    Serial.print(peakToPeak);
    Serial.print(" floor=");        Serial.print(noiseFloor, 1);
    Serial.print(" signal=");       Serial.print(signal, 1);
    Serial.print(" agcMax=");       Serial.print(signalMax, 1);
    Serial.print(" level=");        Serial.print(normalizedLevel, 3);

    const uint16_t litCount = min<uint16_t>(
        static_cast<uint16_t>(roundf(normalizedLevel * LED_COUNT)),
        LED_COUNT);

    Serial.print(" lit=");      Serial.print(litCount);
    Serial.print('/');          Serial.println(LED_COUNT);
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("Sound Reactive WS2812 Meter starting...");

    soundMeter.begin();

    FastLED.addLeds<LED_CHIPSET, LED_DATA_PIN, LED_COLOR_ORDER>(leds, LED_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear(true);

    Serial.println("Ready.");
}

void loop()
{
    // fill_solid(leds, LED_COUNT, CRGB::Black);

    // for (uint16_t i = 0; i < LED_COUNT; ++i)
    // {
    //     if ((i * 3) < LED_COUNT)
    //     {
    //         leds[i] = CRGB::Green;
    //     }
    //     else if ((i * 3) < (LED_COUNT * 2))
    //     {
    //         leds[i] = CRGB::Yellow;
    //     }
    //     else
    //     {
    //         leds[i] = CRGB::Red;
    //     }
    // }

    // FastLED.show();
    // delay(1000);

    // showVolumeBar(1.0f);
    // delay(100);

    uint16_t peakToPeak = 0;
    float noiseFloor = 0.0f;
    float signalMax = 0.0f;

    const float normalizedLevel = soundMeter.readNormalizedLevel(peakToPeak, noiseFloor, signalMax);
    //showVolumeBar(1.0f);

    showVolumeBar(normalizedLevel);
    printDebug(peakToPeak, noiseFloor, signalMax, normalizedLevel);
    delay(1);
}
