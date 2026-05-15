# SoundReactiveWs2812

A PlatformIO / VS Code project for an ESP32-S3 N4R2 that drives a WS2812/WS2812B LED strip from ambient sound level.

This version uses automatic gain control (AGC), so the display adapts to room noise instead of needing a fixed maximum sound value.

## Hardware

- ESP32-S3 N4R2 module or development board
- WS2812 / WS2812B LED strip
- MAX4466 electret microphone amplifier module
- External 5V supply for the LED strip if the strip has more than a few LEDs
- Recommended: 330-470 ohm resistor in series with the LED data line
- Recommended: 1000 uF capacitor across LED strip 5V and GND near the strip

## Default wiring

Edit `include/AppConfig.h` if you want different pins.

| ESP32-S3 | Device |
|---|---|
| GPIO5 | WS2812 DIN, preferably through 330-470 ohm resistor |
| GPIO4 | MAX4466 OUT |
| 3.3V | MAX4466 VCC |
| GND | MAX4466 GND |
| 5V external supply + | WS2812 +5V |
| External supply GND | WS2812 GND and ESP32 GND |

Important: all grounds must be connected together.

## Behavior

The code uses the strip as a proportional bar graph. The number of lit LEDs is proportional to the mapped sound level, while the color zones are fixed by LED position:

- 0% sound level: 0 LEDs lit
- 33% sound level: about the first third lit, colored green
- 66% sound level: about the first two thirds lit, green then yellow
- 100% sound level: all LEDs lit, green then yellow then red

For example, with 60 LEDs and an 80% sound level, 48 LEDs are lit: 20 green, 20 yellow, and 8 red.

## Automatic gain control

The firmware samples the microphone waveform and calculates a peak-to-peak value for each short sample window.

It then tracks two moving values:

1. `noiseFloor` - the quiet-room baseline.
2. `agcMax` - the current loud reference above the noise floor.

The LED level is calculated approximately like this:

```cpp
level = (peakToPeak - noiseFloor - noiseGate) / agcMax;
```

The noise floor moves down fairly quickly when the room gets quieter, but moves up very slowly when the room gets louder. This prevents speech or music from immediately becoming the new silence level.

The AGC maximum rises quickly when a louder sound is detected, then falls slowly as the room gets quieter. This makes the meter usable in both quiet and loud environments.

## Tuning

Open `include/AppConfig.h`.

Common values to change:

```cpp
static constexpr uint8_t LED_DATA_PIN = 5;
static constexpr uint8_t MIC_ADC_PIN  = 4;
static constexpr uint16_t LED_COUNT = 60;
static constexpr uint8_t LED_BRIGHTNESS = 80;
```

Useful AGC tuning values:

```cpp
static constexpr float AGC_MIN_SIGNAL_MAX = 120.0f;
static constexpr float NOISE_GATE_ABOVE_FLOOR = 8.0f;
static constexpr float AGC_ATTACK = 0.25f;
static constexpr float AGC_RELEASE = 0.004f;
static constexpr float VOLUME_SMOOTHING = 0.30f;
```

If the LEDs twitch too much in a quiet room, increase `NOISE_GATE_ABOVE_FLOOR` or `AGC_MIN_SIGNAL_MAX`.

If the display is too slow to react, increase `AGC_ATTACK` or `VOLUME_SMOOTHING`.

If the display stays compressed after a loud sound, increase `AGC_RELEASE` slightly.

## Serial monitor

Open the PlatformIO Serial Monitor at 115200 baud. You will see output similar to:

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

## Build and upload

From PlatformIO in VS Code:

1. Open this folder.
2. Connect the ESP32-S3 board.
3. Build.
4. Upload.
5. Open Serial Monitor at 115200 baud.

## Notes

The board file in `boards/esp32-s3-n4r2.json` defines a generic ESP32-S3 N4R2 board with 4 MB flash and 2 MB PSRAM. If your board already has a matching PlatformIO board ID, you can replace the `board = esp32-s3-n4r2` line in `platformio.ini`.
