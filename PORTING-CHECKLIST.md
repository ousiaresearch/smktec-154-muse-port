# Porting checklist — SMKTelec ESP32-S3 1.54" Touch LCD

Do these in order when the board arrives. Groundwork in `~/workspace/muse-gadget-port/`
(SDK cloned at `muse-gadget-sdk/`, drafts in `port/`).

## 0. Sanity check the hardware (5 min)

- [ ] Plug in via USB-C. Note the serial port name and USB VID/PID:
  `ls /dev/ttyACM* /dev/ttyUSB*` (Linux) or Device Manager (Windows).
  Expected: native USB → `ttyACM0`, VID `303a` / PID `1001`.
  If a bridge chip appears instead, record it for `ports.py`.
- [ ] Compare the PCB against Waveshare ESP32-S3-Touch-LCD-1.54 photos
  (button positions, screen size, mic holes). Clones sometimes move the
  buttons — if PWR/VOL_UP/BOOT aren't where expected, update
  `TALK_GPIO`/`AUX_GPIO` in the board file.

## 1. Apply the port files

- [ ] Run `port/apply.sh` (copies overlay + board file, applies Kconfig /
  CMakeLists / component edits, adds the `154` build alias).
- [ ] Fill in the USB VID/PID in `tools/muse/ports.py` and the alias in
  `tools/muse/avatar.py` (see INTEGRATION.md §6).

## 2. Build

- [ ] `cd muse-gadget-sdk/esp32 && tools/muse/board.sh build 154`
      (fresh `build-154` directory).
- [ ] Resolve every `VERIFY` in `board_smktec_s3_touch_lcd_154.c`:
  - ES7210 API names vs `esp_codec_dev` v1.5 headers.
  - `esp_lcd_touch_cst816s` component API vs resolved version.
  - `ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG` signature.
- [ ] Confirm each overlay line landed in `build-154/sdkconfig`
  (script in devices/AGENTS.md §8).

## 3. Flash and boot

- [ ] Back up factory firmware first:
  `esptool.py --chip esp32s3 -p PORT read-flash 0 0x1000000 factory-backup.bin`
- [ ] `tools/muse/board.sh flash 154 PORT`
- [ ] `tools/muse/monitor.py PORT 30` — expect `link.main: Muse Gadget starting`,
  PSRAM found, no panic loop, BLE advertising as `MuseGadget-XXXXXX`.

## 4. Hardware bring-up (in order)

- [ ] **Display:** logo/avatar appears, colors correct.
  If inverted/wrong order → flip `esp_lcd_panel_invert_color` / `rgb_ele_order`.
- [ ] **Backlight:** brightness slider works (Settings).
- [ ] **Touch:** taps register on the settings screen.
  If dead → check CST816S I2C addr (scan bus), INT/RST levels.
- [ ] **Buttons:** PWR = push-to-talk, BOOT = aux. Check with console `>buttons`
  if available, else watch the log.
- [ ] **Speaker:** Muse replies play out loud. If silent → check PA_EN (GPIO7)
  polarity and ES8311 init in the boot log.
- [ ] **Mics:** voice notes transcribe. If garbled/silent → check ES7210 init,
  `mic_slot` (-1 mixes both).
- [ ] **Battery:** `>power` on console; calibrate ADC levels in `read_power`
  against a multimeter.

## 5. Pair and finish

- [ ] Pair with the Muse app over BLE, join Wi-Fi.
- [ ] Grab an SDK token from gadgets.muse.ai, complete setup.
- [ ] Voice chat works end-to-end; images from Muse show on screen.
- [ ] If it all works: consider upstreaming — the SDK invites new-board
  contributions via the Gadgets Discord.

## Known risks

- Clone pin differences (mitigated by step 0).
- CST816S variant quirks (some clones ship CST816T — same driver, different
  default I2C addr `0x15`; bus scan will reveal).
- ES7210 init sequence differences vs the 1.75C's BSP path.
