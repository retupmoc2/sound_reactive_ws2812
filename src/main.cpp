#include <Arduino.h>
#include <FastLED.h>
#include "AppConfig.h"

CRGB leds[LED_COUNT];

class SoundLevelMeter
{
public:
    void begin()
    {
        analogReadResolution(ADC_BITS); // Set ADC resolution to 12 bits (0-4095) for better sensitivity.
        
        // 11 dB attenuation supports a wider input voltage range on ESP32 ADC pins.
        // The MAX4466 is powered from 3.3V, so its output is safe for the ESP32 ADC.
        analogSetPinAttenuation(MIC_ADC_PIN, ADC_11db);

        _noiseFloor = AGC_INITIAL_NOISE_FLOOR;
        _signalMax = AGC_INITIAL_SIGNAL_MAX;
        _smoothedLevel = 0.0f;
    }

    /* Reads samples from the microphone for a fixed time window and calculates the peak-to-peak amplitude.
        * @return The peak-to-peak amplitude of the sound signal during the sample window.
        * Note: This method is blocking and should be called at a rate that allows for the specified sample window duration.
    */
    uint16_t readPeakToPeak()
    {
        const uint32_t startMs = millis();
        uint16_t minSample = ADC_MAX_VALUE;
        uint16_t maxSample = 0;

        while ((millis() - startMs) < SAMPLE_WINDOW_MS)
        {
            const uint16_t sample = static_cast<uint16_t>(analogRead(MIC_ADC_PIN));
            if (sample < minSample)
                minSample = sample;
            if (sample > maxSample)
                maxSample = sample;
        }

        return maxSample - minSample;
    }

    /*
    * Reads the current sound level and normalizes it to a 0.0 - 1.0 range, applying noise gating,
    * automatic gain control, and smoothing.
    * @param peakToPeakOut Reference to store the peak-to-peak value.
    * @param noiseFloorOut Reference to store the noise floor value.
    * @param signalMaxOut Reference to store the signal maximum value.
    * @return The normalized sound level.
    */
    float readNormalizedLevel(uint16_t &peakToPeakOut, float &noiseFloorOut, float &signalMaxOut)
    {
        const uint16_t peakToPeak = readPeakToPeak();
        peakToPeakOut = peakToPeak;

        updateNoiseFloor(static_cast<float>(peakToPeak));

        float signalAboveFloor = static_cast<float>(peakToPeak) - _noiseFloor - NOISE_GATE_ABOVE_FLOOR;
        if (signalAboveFloor < 0.0f)
        {
            signalAboveFloor = 0.0f;
        }

        updateSignalMax(signalAboveFloor);

        float rawLevel = signalAboveFloor / _signalMax;
        rawLevel = constrain(rawLevel, 0.0f, 1.0f);

        _smoothedLevel = _smoothedLevel + ((rawLevel - _smoothedLevel) * VOLUME_SMOOTHING);

        noiseFloorOut = _noiseFloor;
        signalMaxOut = _signalMax;
        return constrain(_smoothedLevel, 0.0f, 1.0f);
    }

private:
    /*
     * Updates the noise "floor" based on the current peak-to-peak value.
     * @param peakToPeak The current peak-to-peak value.
     */
    void updateNoiseFloor(float peakToPeak)
    {
        if (peakToPeak < _noiseFloor)
        {
            // The room appears quieter. Let the floor move down fairly quickly.
            _noiseFloor = _noiseFloor + ((peakToPeak - _noiseFloor) * NOISE_FLOOR_DECAY_WHEN_QUIETER);
        }
        else
        {
            // The room appears louder. Move the floor up very slowly so speech/music
            // does not immediately become the new baseline.
            _noiseFloor = _noiseFloor + ((peakToPeak - _noiseFloor) * NOISE_FLOOR_RISE_WHEN_LOUDER);
        }

        if (_noiseFloor < 0.0f)
        {
            _noiseFloor = 0.0f;
        }
    }

    /*
     * Updates the maximum signal level based on the current signal above the noise floor.
     * @param signalAboveFloor The current signal level above the noise floor.
     */
    void updateSignalMax(float signalAboveFloor)
    {
        if (signalAboveFloor > _signalMax)
        {
            _signalMax = _signalMax + ((signalAboveFloor - _signalMax) * AGC_ATTACK);
        }
        else
        {
            _signalMax = _signalMax + ((signalAboveFloor - _signalMax) * AGC_RELEASE);
        }

        if (_signalMax < AGC_MIN_SIGNAL_MAX)
        {
            _signalMax = AGC_MIN_SIGNAL_MAX;
        }
    }

    float _noiseFloor = AGC_INITIAL_NOISE_FLOOR;
    float _signalMax = AGC_INITIAL_SIGNAL_MAX;
    float _smoothedLevel = 0.0f;
};

SoundLevelMeter soundMeter;

void showVolumeBar(float level)
{
    fill_solid(leds, LED_COUNT, CRGB::Black);

    const uint16_t firstThirdEnd = LED_COUNT;
    const uint16_t secondThirdEnd = LED_COUNT * 2;

    // Proportional bar graph:
    // - 0.0 lights 0 LEDs
    // - 1.0 lights all LEDs
    // The color zones are fixed by LED position.
    const uint16_t litCount = static_cast<uint16_t>(roundf(level * LED_COUNT));
    const uint16_t clampedLitCount = min<uint16_t>(litCount, LED_COUNT);

    for (uint16_t i = 0; i < clampedLitCount; ++i)
    {
        if ((i * 3) < firstThirdEnd)
        {
            leds[i] = CRGB::Green;
        }
        else if ((i * 3) < secondThirdEnd)
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
    uint16_t peakToPeak = 0;
    float noiseFloor = 0.0f;
    float signalMax = 0.0f;

    const float normalizedLevel = soundMeter.readNormalizedLevel(peakToPeak, noiseFloor, signalMax);

    showVolumeBar(normalizedLevel);
    printDebug(peakToPeak, noiseFloor, signalMax, normalizedLevel);
}
