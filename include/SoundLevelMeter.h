#pragma once

#include "AppConfig.h"

class SoundLevelMeter
{
public:
    void begin();

    uint16_t readPeakToPeak();

    float readNormalizedLevel(uint16_t &peakToPeakOut, float &noiseFloorOut, float &signalMaxOut);

private:
    void updateNoiseFloor(float peakToPeak);
    void updateSignalMax(float signalAboveFloor);

    float _noiseFloor = AGC_INITIAL_NOISE_FLOOR;
    float _signalMax = AGC_INITIAL_SIGNAL_MAX;
    float _smoothedLevel = 0.0f;
};
