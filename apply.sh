#!/bin/bash
# Applies the SMKTelec 1.54" port into a Muse Gadgets SDK checkout.
#
# Usage: ./apply.sh [SDK_ROOT]
#   SDK_ROOT = path to the muse-gadget-sdk checkout (the directory that
#              contains esp32/). Defaults to the old sibling layout:
#              <this-repo>/../muse-gadget-sdk
#
# Safe to re-run (every patch is guarded; copies overwrite).
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
if [ $# -ge 1 ]; then
  ROOT="$(cd "$1" && pwd)"
else
  ROOT="$(cd "$HERE/.." && pwd)/muse-gadget-sdk"
fi
SDK="$ROOT/esp32"
[ -d "$SDK/components/muse" ] || { echo "not an SDK checkout: $SDK"; exit 1; }

echo "== SDK: $SDK =="

echo "== copying overlay =="
cp "$HERE/sdkconfig.muse-smktec-s3-touch-lcd-154" "$SDK/devices/"

echo "== copying board file =="
cp "$HERE/board_smktec_s3_touch_lcd_154.c" "$SDK/components/muse/boards/"

echo "== copying IMU engine =="
cp "$HERE/muse_imu.c" "$HERE/muse_imu.h" "$SDK/components/muse/"

echo "== copying gate + identity =="
cp "$HERE/muse_gate.c" "$HERE/muse_gate.h" "$SDK/components/muse/"
cp "$HERE/muse_identity.c" "$HERE/muse_identity.h" "$SDK/components/muse/"

echo "== copying avatar renderer =="
mkdir -p "$SDK/components/muse/avatar"
cp "$HERE/avatar/muse_pixel.c" "$SDK/components/muse/avatar/muse_pixel.c"
echo "(components/muse/avatar/ is gitignored; the build prefers it over the default)"

echo "== Kconfig =="
KCF="$SDK/components/muse/Kconfig"
if ! grep -q "MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154" "$KCF"; then
  python3 - "$KCF" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
s = s.replace(
  '''        config MUSE_BOARD_AIPI
            bool "AIPI Lite (ESP32-S3, two buttons, no touch)"
            depends on IDF_TARGET_ESP32S3''',
  '''        config MUSE_BOARD_AIPI
            bool "AIPI Lite (ESP32-S3, two buttons, no touch)"
            depends on IDF_TARGET_ESP32S3
        config MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154
            bool "SMKTelec ESP32-S3-Touch-LCD-1.54 (Waveshare clone)"
            depends on IDF_TARGET_ESP32S3''')
s = s.replace(
  '''        default "aipi" if MUSE_BOARD_AIPI''',
  '''        default "aipi" if MUSE_BOARD_AIPI
        default "smktec_s3_touch_lcd_154" if MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154''')
open(p, 'w').write(s)
print("Kconfig updated")
EOF
else
  echo "Kconfig already patched"
fi

echo "== CMakeLists.txt =="
CMK="$SDK/components/muse/CMakeLists.txt"
if ! grep -q "board_smktec_s3_touch_lcd_154" "$CMK"; then
  python3 - "$CMK" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
s = s.replace(
  '''    elseif(CONFIG_MUSE_BOARD_AIPI)
        list(APPEND srcs "boards/board_aipi.c")''',
  '''    elseif(CONFIG_MUSE_BOARD_AIPI)
        list(APPEND srcs "boards/board_aipi.c")
    elseif(CONFIG_MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154)
        list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c" "muse_gate.c" "muse_identity.c")''')
open(p, 'w').write(s)
print("CMakeLists updated")
EOF
else
  echo "CMakeLists already patched (board branch)"
fi
if ! grep -q '"muse_gate.c"' "$CMK"; then
  python3 - "$CMK" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
# Fresh board branch (no extra srcs yet) or the older imu-only form.
s = s.replace(
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c")',
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c" "muse_gate.c" "muse_identity.c")')
s = s.replace(
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c")',
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c" "muse_gate.c" "muse_identity.c")')
open(p, 'w').write(s)
print("CMakeLists updated (muse_gate.c, muse_identity.c)")
EOF
else
  echo "CMakeLists already patched (muse_gate.c)"
fi

echo "== idf_component.yml =="
YML="$SDK/components/muse/idf_component.yml"
if ! grep -q "esp_lcd_touch_cst816s" "$YML"; then
  python3 - "$YML" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
s = s.replace(
  '''  espressif/esp_lcd_touch_spd2010:
    version: "^2.0.1"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\"sensecap_watcher\\""''',
  '''  espressif/esp_lcd_touch_spd2010:
    version: "^2.0.1"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\"sensecap_watcher\\""
  espressif/esp_lcd_touch_cst816s:
    version: "^1.0.0"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\"smktec_s3_touch_lcd_154\\""''')
# extend the lvgl adapter board list
s = s.replace(
  '\\"m5stack_stickc_plus2\\"]"',
  '\\"m5stack_stickc_plus2\\", \\"smktec_s3_touch_lcd_154\\"]"')
open(p, 'w').write(s)
print("idf_component.yml updated (cst816s)")
EOF
else
  echo "idf_component.yml already patched (cst816s)"
fi
if ! grep -q "waveshare/qmi8658" "$YML"; then
  python3 - "$YML" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
s = s.replace(
  '''  espressif/esp_lcd_touch_cst816s:
    version: "^1.0.0"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\"smktec_s3_touch_lcd_154\\""''',
  '''  espressif/esp_lcd_touch_cst816s:
    version: "^1.0.0"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\"smktec_s3_touch_lcd_154\\""
  waveshare/qmi8658:
    version: "==1.0.0"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\"smktec_s3_touch_lcd_154\\""''')
open(p, 'w').write(s)
print("idf_component.yml updated (qmi8658)")
EOF
else
  echo "idf_component.yml already patched (qmi8658)"
fi

echo "== tools/muse/board.sh =="
BSH="$SDK/tools/muse/board.sh"
if ! grep -q "smktec-s3-touch-lcd-154" "$BSH"; then
  python3 - "$BSH" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
s = s.replace(
  "    aipi)    profile=aipi;                 target=esp32s3 ;;",
  "    aipi)    profile=aipi;                 target=esp32s3 ;;\n    154)     profile=smktec-s3-touch-lcd-154; target=esp32s3 ;;")
open(p, 'w').write(s)
print("board.sh updated")
EOF
else
  echo "board.sh already patched"
fi
if ! grep -q "s3|aipi|154" "$BSH"; then
  python3 - "$BSH" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
s = s.replace(
  "build|flash <s3|aipi|c6|watcher|sticks3|plus2>",
  "build|flash <s3|aipi|154|c6|watcher|sticks3|plus2>")
s = s.replace(
  "board=${2:?s3|aipi|c6|watcher|sticks3|plus2}",
  "board=${2:?s3|aipi|154|c6|watcher|sticks3|plus2}")
open(p, 'w').write(s)
print("board.sh usage updated")
EOF
else
  echo "board.sh usage already patched"
fi

echo ""
echo "Done. Still manual:"
echo "  - board file: wire muse_imu_init(s_i2c) + gesture->mode mapping (see INTEGRATION.md)"
echo "  - tools/muse/ports.py: USB VID/PID for alias 154 (needs the board plugged in)"
echo "  - tools/muse/avatar.py: BOARDS mapping"
echo "  - docs tables (devices/README.md, AGENTS.md, README.md)"
