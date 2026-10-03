# Muse Gadget port — SMKTelec ESP32-S3 1.54" Touch LCD

Port of the Muse Gadgets ESP32 SDK (`facebookincubator/muse-gadget-sdk`)
to the SMKTelec ESP32-S3-Touch-LCD-1.54 (Waveshare clone).

## Quick start (on your Mac)

1. Install ESP-IDF **v6.0.1** (see `MAC_SETUP.md`).
2. `git clone https://github.com/facebookincubator/muse-gadget-sdk`
3. `git clone https://github.com/ousiaresearch/smktec-154-muse-port`
4. `cd smktec-154-muse-port && ./apply.sh ../muse-gadget-sdk`
5. `cd ../muse-gadget-sdk/esp32 && tools/muse/board.sh build 154`

Full guide: [`MAC_SETUP.md`](MAC_SETUP.md). Bring-up order:
[`PORTING-CHECKLIST.md`](PORTING-CHECKLIST.md).

## What's here

| File | Purpose |
|---|---|
| `PINOUT.md` | Full pin table with sources (xiaozhi-esp32, Waveshare spec) |
| `sdkconfig.muse-smktec-s3-touch-lcd-154` | Draft Kconfig overlay (S3R8, 16 MB flash, octal PSRAM) |
| `board_smktec_s3_touch_lcd_154.c` | Draft board driver (ST7789 + CST816S touch + ES7210/ES8311 audio) |
| `INTEGRATION.md` | Every SDK-tree edit needed (Kconfig, CMake, components, tools, docs) |
| `PORTING-CHECKLIST.md` | Ordered steps for bring-up day |
| `apply.sh` | Copies drafts into the SDK tree and applies mechanical edits (verified) |

## Status

`apply.sh` has been run once against the clone — all patches applied cleanly,
YAML valid. Drafts still carry `VERIFY`/`TODO` markers for things only the
physical board can settle: USB VID/PID, exact button layout, ES7210/CST816S
API details at build time, battery ADC calibration, color invert flag.

## Key facts

- 8 MB octal PSRAM → full feature set possible (tunnel, voice, images, OTA).
- Display driver (ST7789) already in the SDK; audio chips (ES7210+ES8311)
  match the supported 1.75C board; only genuinely new piece is the CST816S
  touch component.
- Build alias: `154` → `tools/muse/board.sh build 154`.
