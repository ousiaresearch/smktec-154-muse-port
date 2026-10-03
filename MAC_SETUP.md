# Mac setup guide — building the SMKTelec 1.54" port

Do this on your Mac after the board arrives. Lapis did the port groundwork;
this is the machine-side half. Expect ~1 hour the first time, mostly waiting
on downloads and the first compile.

## 0. What you need

- Your Mac (Apple Silicon is fine), the board, a USB-C **data** cable.
- Your Muse SDK token: https://gadgets.muse.ai → Account → SDK tokens.
  It looks like `mgst_…`. One token covers up to 50 devices.
- The port repo: https://github.com/ousiaresearch/smktec-154-muse-port
  (board driver, sdkconfig overlay, avatar renderer, IMU engine, apply
  script, and all the docs). Clone it next to the SDK.

## 1. Install ESP-IDF v6.0.1 (exact version matters)

Easiest: install VS Code, then the **ESP-IDF extension**, and tell it to
install **ESP-IDF v6.0.1** (not v5, not latest). It lands in `~/esp/esp-idf-v6.0.1`,
which the build scripts find automatically.

Command-line alternative:
```
mkdir -p ~/esp && cd ~/esp
git clone -b v6.0.1 --recursive https://github.com/espressif/esp-idf.git esp-idf-v6.0.1
cd esp-idf-v6.0.1 && ./install.sh esp32s3
```
Then in every new shell before building: `. ~/esp/esp-idf-v6.0.1/export.sh`

You need Python 3 and Xcode command-line tools (`xcode-select --install`).

## 2. Get the SDK and the port files

```
git clone https://github.com/facebookincubator/muse-gadget-sdk
git clone https://github.com/ousiaresearch/smktec-154-muse-port
cd smktec-154-muse-port
./apply.sh ../muse-gadget-sdk   # copies overlay + board file, patches Kconfig/CMakeLists
```

## 3. Build

```
cd muse-gadget-sdk/esp32
tools/muse/board.sh build 154
```

First build downloads managed components and takes 10–20 minutes. If it
complains about the SDK token, set it in the build dir:
```
# in build-154/sdkconfig, or via: idf.py -B build-154 menuconfig
CONFIG_GADGET_SDK_TOKEN="mgst_…"
```
then rebuild. Never commit or share the token.

## 4. Plug in the board, find the port

```
ls /dev/cu.usbmodem*        # native USB (expected): 303a:1001
ls /dev/cu.usbserial*       # bridge chip (CH340/CP210x) — needs vendor driver
```
Bridge-chip drivers on macOS need one approval in
System Settings → Privacy & Security after install. Tell Lapis the VID/PID
and he'll add it to `tools/muse/ports.py`.

## 5. Back up, flash, watch it boot

```
# back up all 16 MB of factory firmware FIRST
python -m esptool --chip esp32s3 -p PORT -b 460800 read-flash 0 0x1000000 factory-backup.bin

tools/muse/board.sh flash 154 PORT
tools/muse/monitor.py PORT 30     # expect: link.main: Muse Gadget starting
```

## 6. If the build fails

Copy the error output and send it to Lapis — most likely candidates are the
`VERIFY` items in the board file (touch/codec API names). He'll fix the
draft, you'll re-run `./port/apply.sh`, rebuild.

## After it boots

Pair it in the Muse app (it advertises as `MuseGadget-XXXXXX`, breathing
orange = waiting for setup). Then the fun checklist: display, touch,
buttons, speaker, mics, IMU gestures (shake me), BLE/Wi-Fi, OTA.
