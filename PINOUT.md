# Pinout — SMKTelec ESP32-S3 1.54" Touch LCD (Waveshare ESP32-S3-Touch-LCD-1.54 clone)

The SMKTelec board is an off-brand of Waveshare's ESP32-S3-Touch-LCD-1.54
(part 33868). Pins below come from xiaozhi-esp32's board definition for that
exact model (`main/boards/waveshare/esp32-s3-touch-lcd-1.54/config.h`) and the
Waveshare spec sheet. **Verify on arrival** — clones usually copy the
reference design pin-for-pin, but one wrong pin = silent failure.

## Chip / memory / USB

| Item | Value |
|---|---|
| SoC | ESP32-S3R8 (dual LX7 @ 240 MHz, **8 MB octal PSRAM**) |
| Flash | 16 MB |
| USB | Almost certainly native USB Serial/JTAG (Type-C direct). Verify: expect `/dev/ttyACM0` on Linux. If it shows as `ttyUSB*`, note the bridge chip. |

8 MB octal PSRAM means the full feature set is possible: home-network
tunnel, voice sessions, images from Muse, OTA.

## Display — ST7789, 1.54" 240×240 IPS, capacitive touch

| Signal | GPIO | Notes |
|---|---|---|
| SPI CS | 21 | SPI mode 3 |
| SPI MOSI | 39 | |
| SPI CLK | 38 | |
| DC | 45 | |
| RST | 40 | |
| Backlight | 46 | Active high (verify) |
| Resolution | 240×240 | Invert colors = true, RGB order (per xiaozhi config) |

Driver `esp_lcd` ST7789 already exists in the Muse SDK (used by the
ideaspark board). No new display driver needed.

## Touch — CST816S (I2C)

| Signal | GPIO | Notes |
|---|---|---|
| INT | 48 | |
| RST | 47 | |
| I2C bus | SDA 42 / SCL 41 | Shared with audio codec |

Needs `espressif/esp_lcd_touch_cst816s` added as a managed component
(not currently in the SDK — see INTEGRATION.md).

## Audio — ES7210 (dual mic) + ES8311 (codec) + speaker

| Signal | GPIO | Notes |
|---|---|---|
| I2S MCLK | 8 | |
| I2S BCLK | 9 | |
| I2S WS | 10 | |
| I2S DIN (mic → S3) | 11 | |
| I2S DOUT (S3 → speaker) | 12 | |
| PA enable (amp) | 7 | Drive high to enable speaker amp |
| Codec I2C | SDA 42 / SCL 41 | Shared with touch |

Same audio chips as the supported Waveshare 1.75C board — the audio path
is proven in the SDK, just wired to different pins here.

## Buttons

| Button | GPIO | Notes |
|---|---|---|
| PWR | 5 | → push-to-talk |
| VOL_UP | 4 | → aux/menu |
| VOL_DOWN / BOOT | 0 | BOOT on S3 |

## Power / battery

| Signal | GPIO | Notes |
|---|---|---|
| Battery enable | 2 | Drive high in `init()` (xiaozhi `PowerON()`) |
| Battery ADC | 1 | Voltage divider; needs calibration on hardware |
| Charging indicator | 3 | High while charging |

## Extras (not needed for the port)

QMI8658 6-axis IMU (I2C, SDA 42 / SCL 41 shared bus, addr 0x6B VERIFY), microSD slot, 3.7V battery charge circuit.
IMU is IN SCOPE: drives the gesture engine (muse_imu) and Lapis expressions. See EXPRESSIONS.md.

## Sources

- xiaozhi-esp32 `main/boards/waveshare/esp32-s3-touch-lcd-1.54/config.h`
  (pin definitions) and `esp32-s3-touch-lcd-1.54.cc` (ST7789 + CST816S init)
- Waveshare ESP32-S3-Touch-LCD-1.54 spec (sunsky-online listing): ST7789
  driver IC, CST816 touch IC, ES7210 + ES8311 + NS4150B audio, 8 MB PSRAM,
  16 MB flash
