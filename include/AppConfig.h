#pragma once

#include <Arduino.h>

// -------------------------
// Hardware configuration
// -------------------------
// Change these pins to match your board/wiring.
// Avoid ESP32-S3 strapping pins and pins already used by USB/JTAG/PSRAM/flash on your board.
static constexpr uint8_t LED_DATA_PIN = 5;
static constexpr uint8_t MIC_ADC_PIN  = 13;     // ADC-capable pin connected to MAX4466 OUT

static constexpr uint16_t LED_COUNT = 10;
static constexpr uint8_t LED_BRIGHTNESS = 80;

// WS2812 / NeoPixel strips are normally GRB.
#define LED_CHIPSET WS2812B
#define LED_COLOR_ORDER GRB

// -------------------------
// Audio sampling
// -------------------------
// Audio sample window. 20 ms is responsive but still catches enough waveform peaks.
static constexpr uint16_t SAMPLE_WINDOW_MS = 20;

// ADC configuration. ESP32 ADC readings are 0..4095 at 12-bit resolution.
static constexpr uint8_t ADC_BITS = 12;
static constexpr uint16_t ADC_MAX_VALUE = (1 << ADC_BITS) - 1;

// -------------------------
// Automatic gain control / adaptation
// -------------------------
// The firmware estimates two moving values:
//   noise floor: typical quiet-room peak-to-peak value
//   signal max:  a moving loud reference above the noise floor
// The LED bar displays (current - noiseFloor) / signalMax.

// Initial estimates. These do not have to be exact; they are learned while running.
static constexpr float AGC_INITIAL_NOISE_FLOOR = 25.0f;
static constexpr float AGC_INITIAL_SIGNAL_MAX  = 300.0f;

// Minimum signal span above the noise floor. Prevents tiny quiet-room changes from
// expanding to full scale and making the display twitchy.
static constexpr float AGC_MIN_SIGNAL_MAX = 120.0f;

// Noise floor learning. It adapts quickly when the room gets quieter, but very slowly
// when the room gets louder so music/speech is not mistaken for the new silence level.
static constexpr float NOISE_FLOOR_DECAY_WHEN_QUIETER = 0.02f;
static constexpr float NOISE_FLOOR_RISE_WHEN_LOUDER   = 0.0008f;

// Signal max learning. It rises quickly for louder sounds, then falls slowly so the
// display remains useful as the room gets quieter.
static constexpr float AGC_ATTACK = 0.25f;
static constexpr float AGC_RELEASE = 0.004f;

// Noise gate: how far above the learned noise floor a sound must be before it lights LEDs.
static constexpr float NOISE_GATE_ABOVE_FLOOR = 8.0f;

// Smoothing prevents LED flicker. Larger = more responsive, smaller = smoother/slower.
static constexpr float VOLUME_SMOOTHING = 0.30f;

// Serial debug output interval.
static constexpr uint16_t DEBUG_PRINT_MS = 250;

//static constexpr uint16_t LED_FRAME_GAP_SPACE = 250;    // value in microseconds to space out LED frame updates.

