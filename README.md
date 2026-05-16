# Sound-Reactive WS2812 Meter

A PlatformIO / VS Code project for driving a WS2812/WS2812B LED strip from ambient sound using an ESP32-S3 board and a MAX4466 analog microphone module.

The display behaves like a proportional audio level meter:

- Low levels light the first part of the strip green.
- Medium levels extend into the yellow region.
- High levels extend into the red region.
- The number of lit LEDs is proportional to the current normalized sound level.

This version includes automatic gain control (AGC), so it adapts to the room noise level instead of requiring a fixed maximum sound value.

---

## Hardware used by the current configuration

- Waveshare ESP32-S3 Zero / ESP32-S3 N4R2 style board
- WS2812 / WS2812B LED strip
- MAX4466 electret microphone amplifier module
- External 5V supply for the LED strip, if powering more than a few LEDs
- Common ground between the ESP32 board, LED strip supply, and microphone module

Recommended LED strip protection:

- 330–470 ohm resistor in series with the WS2812 data line
- 1000 uF electrolytic capacitor across LED strip +5V and GND near the strip input
- A 74AHCT125 or 74HCT245 level shifter if the strip is unreliable from a 3.3V ESP32 data signal

---

## Default wiring

The current pin definitions are in `include/AppConfig.h`.

| ESP32-S3 pin | Connection |
|---|---|
| GPIO5 | WS2812 DIN, preferably through a 330–470 ohm resistor |
| GPIO13 | MAX4466 OUT |
| 3.3V | MAX4466 VCC |
| GND | MAX4466 GND |
| 5V external supply + | WS2812 +5V |
| External supply GND | WS2812 GND and ESP32 GND |

Important wiring notes:

- The MAX4466 OUT signal should connect only to the ADC input pin.
- The WS2812 DIN signal should connect only to the LED data pin.
- The ESP32, microphone module, and LED strip power supply must share ground.
- Do not power a long LED strip from the ESP32 board's 5V pin.

---

## LED behavior

The strip is treated as a fixed-color proportional bar graph.

The color zones are based on LED position:

- First third: green
- Middle third: yellow
- Last third: red

The number of lit LEDs is based on the current normalized sound level.

For example, with 60 LEDs and an 80% sound level:

```text
48 LEDs lit total
20 green
20 yellow
8 red
12 off
```

For a 10 LED test strip, full scale should display approximately:

```text
LEDs 1-4   green
LEDs 5-7   yellow
LEDs 8-10  red
```

The zone logic is intentionally written to work better with short test strips than simple integer division by 3.

---

## Automatic gain control

The firmware samples the microphone waveform for a short window and calculates a peak-to-peak value.

It then tracks two moving values:

1. `noiseFloor` — the quiet-room baseline.
2. `agcMax` — the current loud reference above the noise floor.

The normalized LED level is calculated conceptually like this:

```cpp
level = (peakToPeak - noiseFloor - noiseGate) / agcMax;
```

The noise floor moves down fairly quickly when the room gets quieter, but rises very slowly when the room gets louder. This keeps speech or music from immediately becoming the new silence level.

The AGC maximum rises quickly when a louder sound is detected, then falls slowly as the room quiets down. This lets the meter adapt to both quiet and loud environments.

---

## Timing note

The main loop currently includes a short `delay(1)` after each LED update.

That small delay is intentional. It gives the ESP32 runtime a chance to yield and helps avoid unstable-looking LED updates on the current hardware setup.

If you experiment with replacing it with `delayMicroseconds(...)`, treat that value as an LED/display timing gap, not a microphone tuning value. Different LED strips, wiring, level shifting, and power layouts may behave differently.

---

## Configuration

Open `include/AppConfig.h` to tune the project.

Common hardware settings:

```cpp
static constexpr uint8_t LED_DATA_PIN = 5;
static constexpr uint8_t MIC_ADC_PIN  = 13;
static constexpr uint16_t LED_COUNT = 10;
static constexpr uint8_t LED_BRIGHTNESS = 80;
```

Common audio/AGC settings:

```cpp
static constexpr uint16_t SAMPLE_WINDOW_MS = 20;
static constexpr float AGC_MIN_SIGNAL_MAX = 120.0f;
static constexpr float NOISE_GATE_ABOVE_FLOOR = 8.0f;
static constexpr float AGC_ATTACK = 0.25f;
static constexpr float AGC_RELEASE = 0.004f;
static constexpr float VOLUME_SMOOTHING = 0.30f;
```

Tuning suggestions:

- If LEDs twitch too much in a quiet room, increase `NOISE_GATE_ABOVE_FLOOR`.
- If the meter too easily jumps to full scale, increase `AGC_MIN_SIGNAL_MAX`.
- If the display reacts too slowly to loud sounds, increase `AGC_ATTACK` or `VOLUME_SMOOTHING`.
- If the display stays compressed after a loud sound, increase `AGC_RELEASE` slightly.
- If the strip is too bright while testing, reduce `LED_BRIGHTNESS`.

---

## Serial monitor

Open the PlatformIO Serial Monitor at 115200 baud.

Debug output looks like this:

```text
peakToPeak=184 floor=31.2 signal=144.8 agcMax=421.6 level=0.344 lit=21/60
```

Field meanings:

- `peakToPeak`: raw microphone waveform size for the sample window
- `floor`: learned quiet-room baseline
- `signal`: sound above the learned floor and noise gate
- `agcMax`: learned full-scale reference
- `level`: normalized 0.0 to 1.0 LED level
- `lit`: number of LEDs currently lit

`printDebug(...)` is currently present but not called from `loop()`. Uncomment or add the call in `loop()` when you want serial diagnostics.

---

## Build and upload

1. Open the project folder in VS Code.
2. Install or open the PlatformIO extension.
3. Connect the ESP32-S3 board.
4. Select the `waveshare_esp32_s3_zero` environment.
5. Build.
6. Upload.
7. Open Serial Monitor at 115200 baud if debugging.

The current `platformio.ini` includes multiple environments. The default is:

```ini
[platformio]
default_envs = waveshare_esp32_s3_zero
```

If your board appears on a different serial port, update or remove this line from the selected environment:

```ini
upload_port = COM7
```

---

## Project notes

The project includes `boards/esp32-s3-n4r2.json`, but the current default environment uses PlatformIO's `esp32-s3-devkitc-1` board definition with Waveshare-specific flash/PSRAM settings. The custom board file is therefore not used by the default environment unless `platformio.ini` is changed to reference it.

The source code is intentionally small:

```text
include/AppConfig.h          Hardware and tuning constants
include/SoundLevelMeter.h    Sound meter class declaration
src/SoundLevelMeter.cpp      ADC sampling and AGC logic
src/main.cpp                 FastLED setup, LED bar drawing, main loop
```

