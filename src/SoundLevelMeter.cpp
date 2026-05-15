#include "SoundLevelMeter.h"

#include <Arduino.h>

void SoundLevelMeter::begin()
{
    analogReadResolution(ADC_BITS); // Set ADC resolution to 12 bits (0-4095) for better sensitivity.

    // 11 dB attenuation supports a wider input voltage range on ESP32 ADC pins.
    // The MAX4466 is powered from 3.3V, so its output is safe for the ESP32 ADC.
    analogSetPinAttenuation(MIC_ADC_PIN, ADC_11db);

    _noiseFloor = AGC_INITIAL_NOISE_FLOOR;
    _signalMax = AGC_INITIAL_SIGNAL_MAX;
    _smoothedLevel = 0.0f;
}

uint16_t SoundLevelMeter::readPeakToPeak()
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
 * This method performs the following steps:
    * 1. Reads the peak-to-peak value from the microphone.
    * 2. Updates the noise floor estimate based on the new peak-to-peak value.
    * 3. Calculates the signal level above the noise floor and updates the signal max estimate
 * @param peakToPeakOut Reference to store the peak-to-peak value.
 * @param noiseFloorOut Reference to store the noise floor value.
 * @param signalMaxOut Reference to store the signal max value.
 * @return The normalized sound level.
 */
float SoundLevelMeter::readNormalizedLevel(uint16_t &peakToPeakOut, float &noiseFloorOut, float &signalMaxOut)
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

/*
 * Updates the noise floor estimate based on the new peak-to-peak value.
 * @param peakToPeak The new peak-to-peak value.
 */
void SoundLevelMeter::updateNoiseFloor(float peakToPeak)
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
 * Updates the signal max estimate based on the new signal level. 
 * This is part of the AGC algorithm to adapt to changing sound levels.
 * @param signalAboveFloor The new signal level above the noise floor.
 */
void SoundLevelMeter::updateSignalMax(float signalAboveFloor)
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
