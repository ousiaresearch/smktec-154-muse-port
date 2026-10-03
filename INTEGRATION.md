# Integration — wiring the SMKTelec 1.54" board into the SDK tree

Board ID: `smktec_s3_touch_lcd_154`
Kconfig symbol: `MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154`
Overlay: `devices/sdkconfig.muse-smktec-s3-touch-lcd-154`
Board file: `components/muse/boards/board_smktec_s3_touch_lcd_154.c`
Build alias: `154`

`apply.sh` in this directory performs the file copies and the mechanical
edits (1–4, 7). Steps 5–6 and 8 need values only known once the board is
plugged in (USB VID/PID).

## 1. `components/muse/Kconfig`

Add to the `MUSE_BOARD` choice (after the AIPI entry):

```
        config MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154
            bool "SMKTelec ESP32-S3-Touch-LCD-1.54 (Waveshare clone)"
            depends on IDF_TARGET_ESP32S3
```

Add to the `MUSE_BOARD_ID` defaults (above `default "none"`):

```
        default "smktec_s3_touch_lcd_154" if MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154
```

## 2. `components/muse/CMakeLists.txt`

Add beside the other boards:

```
    elseif(CONFIG_MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154)
        list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c")
```

## 3. `components/muse/idf_component.yml`

Add the CST816S touch component (new — not currently in the SDK):

```yaml
  espressif/esp_lcd_touch_cst816s:
    version: "^1.0.0"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \"smktec_s3_touch_lcd_154\""
```

Extend the `esp_lvgl_adapter` rule's board list with
`\"smktec_s3_touch_lcd_154\"`.

## 4. Overlay

Copy `port/sdkconfig.muse-smktec-s3-touch-lcd-154` →
`muse-gadget-sdk/esp32/devices/sdkconfig.muse-smktec-s3-touch-lcd-154`.

## 5. `tools/muse/board.sh`

Add the alias (next to the other S3 boards):

```
    154)     profile=smktec-s3-touch-lcd-154; target=esp32s3 ;;
```

Add `154` to the usage comment and the `${2:?...}` selector.

## 6. `tools/muse/ports.py` and `tools/muse/avatar.py`

- `ports.py`: map alias `154` to the console's USB VID/PID in `USB`.
  **TODO on arrival:** plug the board in, read VID/PID from the OS
  (expected native USB Serial/JTAG → VID `303a`, PID `1001`; if it shows a
  bridge chip instead, note it here).
- `avatar.py`: map board name → alias in `BOARDS`.

## 7. Docs

- `esp32/devices/README.md`: Supported-devices row, Features column, Build row.
- `esp32/AGENTS.md`: Supported boards table. Add the Waveshare
  ESP32-S3-Touch-LCD-1.54 vendor source row (xiaozhi-esp32 board link).
- `README.md` (repo root): Boards table.

## 8. Verify (per devices/AGENTS.md §8)

Build from scratch, check every overlay line landed in the generated
sdkconfig, run host tests, then flash and watch the boot log for
`link.main: Muse Gadget starting`, PSRAM found, no panic loop, and the
BLE name advertising.

## 9. IMU wiring (gesture engine)

`apply.sh` copies `muse_imu.c/h` into `components/muse/` and adds the
`waveshare/qmi8658` managed component. Still to wire in
`board_smktec_s3_touch_lcd_154.c`, right after the shared I2C bus and touch
are up:

```c
#include "muse_imu.h"

muse_imu_init(s_i2c);              // probe + configure; safe if IMU absent
muse_imu_set_callback(imu_gesture_cb, NULL);
muse_imu_start();                  // 50 Hz task; no-op if IMU absent
```

Gesture → firmware mapping (see EXPRESSIONS.md for the full table):
face-down → `MUSE_MODE_SLEEPY`, face-up → `IDLE`, shake → `DIZZY` (3s),
tap → `muse_state_make_happy()`, tilt → `muse_pixel_set_facing()`.
`SLEEPY`/`DIZZY` need the `muse_mode_t` enum extended in
`components/muse/muse_state.h` (insert before `MUSE_MODE_COUNT`), and the
`muse_pixel_set_facing()` prototype added to `muse_pixel.h`.

## 10. Personalization wiring (identity, brain, diary)

`apply.sh` copies `muse_identity.c/h`, `muse_brain.c/h`, `muse_gate.c/h`,
`muse_diary.c/h` into `components/muse/` and adds them to the board's
`srcs`. The board file wires them:

- **`init()`**: `muse_identity_init()` (NVS mint/load — never fails boot),
  then `muse_diary_init()` (best-effort SD mount), then
  `muse_brain_init(&s_brain, muse_diary_append)` — the diary is the
  brain's log sink. `s_brain_ready` gates everything downstream.
- **`read_power()`** (polled every 2s awake / 10s paused by `muse_input`):
  runs `muse_brain_tick()` with `muse_state_asleep()`. Battery feeds the
  brain only once `battery_mv` is calibrated — until then the brain
  honestly reports stale energy (neutral defaults at the gate).
- **`smktec_note_sleepy(bool entering)`**: call on SLEEPY entry (§9) —
  runs `muse_brain_consolidate()`, the dream pass. Non-static, declared
  implicitly; add a prototype where the SLEEPY implementation lives.
- **`smktec_brain()`**: returns the brain state (or NULL) for the future
  gate middleware in the voice turn loop (SDK core, upstream-PR
  territory).

VERIFY on the Mac build: NVS, FatFS/SDMMC mount (slot pins!), and that
`muse_state_asleep()` is linkable from the board file (same component —
should be).
