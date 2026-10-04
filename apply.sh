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

echo "== copying fixed qmi8658 component (IDF v6: esp_driver_i2c in REQUIRES) =="
mkdir -p "$SDK/components/qmi8658"
cp -r "$HERE/components/qmi8658/." "$SDK/components/qmi8658/"
echo "(local components/qmi8658 overrides the waveshare/qmi8658 registry dep; see components/qmi8658/VENDORING.md)"

echo "== copying gate + identity + brain + diary =="
cp "$HERE/muse_gate.c" "$HERE/muse_gate.h" "$SDK/components/muse/"
cp "$HERE/muse_identity.c" "$HERE/muse_identity.h" "$SDK/components/muse/"
cp "$HERE/muse_brain.c" "$HERE/muse_brain.h" "$SDK/components/muse/"
cp "$HERE/muse_diary.c" "$HERE/muse_diary.h" "$SDK/components/muse/"

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
        list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c" "muse_gate.c" "muse_identity.c" "muse_brain.c" "muse_diary.c")''')
open(p, 'w').write(s)
print("CMakeLists updated")
EOF
else
  echo "CMakeLists already patched (board branch)"
fi
if ! grep -q '"muse_diary.c"' "$CMK"; then
  python3 - "$CMK" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
new = 'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c" "muse_gate.c" "muse_identity.c" "muse_brain.c" "muse_diary.c")'
forms = [
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c")',
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c")',
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c" "muse_gate.c" "muse_identity.c")',
  'list(APPEND srcs "boards/board_smktec_s3_touch_lcd_154.c" "muse_imu.c" "muse_gate.c" "muse_identity.c" "muse_brain.c")',
]
for f in forms:
    s = s.replace(f, new)
open(p, 'w').write(s)
print("CMakeLists updated (muse_diary.c)")
EOF
else
  echo "CMakeLists already patched (muse_diary.c)"
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
      - if: "$CONFIG{MUSE_BOARD_ID} == \\\"sensecap_watcher\\\""''',
  '''  espressif/esp_lcd_touch_spd2010:
    version: "^2.0.1"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\\"sensecap_watcher\\\""
  espressif/esp_lcd_touch_cst816s:
    version: "^1.0.0"
    rules:
      - if: "$CONFIG{MUSE_BOARD_ID} == \\\"smktec_s3_touch_lcd_154\\\""''')
open(p, 'w').write(s)
print("idf_component.yml updated (cst816s)")
EOF
else
  echo "idf_component.yml already patched (cst816s)"
fi
# The lvgl-adapter board list gets its own guard: the old anchor (a neighbor
# board) broke silently when upstream added boards to the list, so this must
# stay re-runnable even after the cst816s patch applied.
if grep -A3 'espressif/esp_lvgl_adapter:' "$YML" | grep -q 'smktec_s3_touch_lcd_154'; then
  echo "idf_component.yml already patched (lvgl_adapter)"
else
  python3 - "$YML" <<'EOF'
import re, sys
p = sys.argv[1]
s = open(p).read()
# Anchor on the dep, never on neighboring boards (upstream changes those).
pat = re.compile(r'(espressif/esp_lvgl_adapter:\n(?:[^\n]*\n)*?      - if: "\$CONFIG\{MUSE_BOARD_ID\} in \[)')
s, n = pat.subn(r'\g<1>\\"smktec_s3_touch_lcd_154\\", ', s, count=1)
assert n == 1, "esp_lvgl_adapter rule anchor not found; refusing to silently skip"
open(p, 'w').write(s)
print("idf_component.yml updated (lvgl_adapter)")
EOF
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

echo "== copying turn gate + ledger + pages =="
cp "$HERE/muse_turn_gate.c" "$HERE/muse_turn_gate.h" "$SDK/components/muse/"
cp "$HERE/muse_ledger.c" "$HERE/muse_ledger.h" "$SDK/components/muse/"
cp "$HERE/muse_vitals_page.c" "$HERE/muse_vitals_page.h" "$SDK/components/muse/"
cp "$HERE/muse_diary_page.c" "$HERE/muse_diary_page.h" "$SDK/components/muse/"

echo "== CMakeLists.txt (turn gate, ledger, pages) =="
CMK="$SDK/components/muse/CMakeLists.txt"
if ! grep -q '"muse_turn_gate.c"' "$CMK"; then
  python3 - "$CMK" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
old = '"muse_brain.c" "muse_diary.c"'
new = '"muse_brain.c" "muse_diary.c" "muse_turn_gate.c" "muse_ledger.c" "muse_vitals_page.c" "muse_diary_page.c"'
assert old in s, "CMakeLists anchor not found"
s = s.replace(old, new)
open(p, 'w').write(s)
print("CMakeLists updated (turn gate, ledger, pages)")
EOF
else
  echo "CMakeLists already patched (turn gate, ledger, pages)"
fi
echo "== CMakeLists.txt (fatfs/sdmmc for muse_diary.c, IDF v6) =="
CMK="$SDK/components/muse/CMakeLists.txt"
if grep -q "esp_driver_pcnt fatfs" "$CMK"; then
  echo "CMakeLists already patched (fatfs)"
else
  python3 - "$CMK" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
old = "    esp_driver_pcnt)"
new = "    esp_driver_pcnt fatfs sdmmc esp_driver_sdmmc)"
assert old in s, "muse_priv_requires anchor not found; refusing to silently skip"
s = s.replace(old, new, 1)
open(p, 'w').write(s)
print("CMakeLists updated (fatfs)")
EOF
fi

echo "== muse_input.c (turn-gate hook) =="
INP="$SDK/components/muse/muse_input.c"
if ! grep -q "muse_turn_gate_veto" "$INP"; then
  python3 - "$INP" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
anchor = "static void talk_button(unsigned ev)"
weak = """/* Turn-gate hook (port overlay): the creature can veto a press.
 * Weak no-op; the port's muse_turn_gate.c overrides it with the real
 * decision gate. A VETO swallows the press the way menu taps do. */
__attribute__((weak)) bool muse_turn_gate_veto(void) { return false; }

"""
assert anchor in s, "talk_button anchor not found"
s = s.replace(anchor, weak + anchor, 1)
old_branch = """        } else {
            post(MUSE_PTT_DOWN, false);
            talk_down = true;
        }"""
new_branch = """        } else if (muse_turn_gate_veto()) {
            muse_state_poke();
            swallow = true;
        } else {
            post(MUSE_PTT_DOWN, false);
            talk_down = true;
        }"""
assert old_branch in s, "PTT_DOWN branch anchor not found"
s = s.replace(old_branch, new_branch, 1)
open(p, 'w').write(s)
print("muse_input.c patched (turn-gate hook)")
EOF
else
  echo "muse_input.c already patched"
fi

echo "== muse_chat_session.cpp (snapshot context hook) =="
SES="$SDK/components/muse/muse_chat_session.cpp"
if ! grep -q "muse_turn_context" "$SES"; then
  python3 - "$SES" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
anchor = "static void send_chat(const char *text, const char *modality)"
weak = """/* Chat context hook (port overlay): the brain snapshot rides along with
 * every /chat/stream message, voice transcripts included. Weak no-op; the
 * port's muse_turn_gate.c overrides it. The cortex is in the cloud; the
 * snapshot is the continuity bridge. */
extern "C" __attribute__((weak)) size_t muse_turn_context(char *out, size_t cap)
{
    (void)out;
    (void)cap;
    return 0;
}

"""
assert anchor in s, "send_chat anchor not found"
s = s.replace(anchor, weak + anchor, 1)
old_body = """    s_turn.chat_posted = true;
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "message", text);"""
new_body = """    s_turn.chat_posted = true;
    char ctx[1152];
    size_t ctx_n = muse_turn_context(ctx, sizeof(ctx) - 1);
    const char *msg = text;
    char *combined = nullptr;
    if (ctx_n > 0) {
        ctx[ctx_n] = '\\0';
        size_t tlen = strlen(text);
        combined = static_cast<char *>(malloc(ctx_n + 2 + tlen + 1));
        if (combined) {
            memcpy(combined, ctx, ctx_n);
            combined[ctx_n] = '\\n';
            combined[ctx_n + 1] = '\\n';
            memcpy(combined + ctx_n + 2, text, tlen + 1);
            msg = combined;
        }
    }
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "message", msg);"""
assert old_body in s, "send_chat body anchor not found"
s = s.replace(old_body, new_body, 1)
old_free = """    cJSON_free(json);
    if (!ok) {"""
new_free = """    cJSON_free(json);
    free(combined);
    if (!ok) {"""
assert old_free in s, "send_chat free anchor not found"
s = s.replace(old_free, new_free, 1)
open(p, 'w').write(s)
print("muse_chat_session.cpp patched (snapshot context hook)")
EOF
else
  echo "muse_chat_session.cpp already patched"
fi

echo "== muse_voice.c (turn-ledger hooks) =="
VOI="$SDK/components/muse/muse_voice.c"
if ! grep -q "muse_ledger_heard" "$VOI"; then
  python3 - "$VOI" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
anchor = "static bool hatch_reply(bool *delivered)"
weak = """/* Turn-ledger hooks (port overlay): every turn lands on the SD card.
 * Weak no-ops; the port's muse_ledger.c overrides them. */
__attribute__((weak)) void muse_ledger_heard(const char *text) { (void)text; }
__attribute__((weak)) void muse_ledger_reply(const char *text) { (void)text; }
__attribute__((weak)) void muse_ledger_turn_end(bool delivered, const char *note)
{
    (void)delivered;
    (void)note;
}

"""
assert anchor in s, "hatch_reply anchor not found"
s = s.replace(anchor, weak + anchor, 1)
subs = [
    ("""            case MUSE_HATCH_EV_HEARD:
                if (!speaking && !replied) {""",
     """            case MUSE_HATCH_EV_HEARD:
                muse_ledger_heard(text);
                if (!speaking && !replied) {"""),
    ("""            case MUSE_HATCH_EV_REPLY:
                replied = *delivered = true;""",
     """            case MUSE_HATCH_EV_REPLY:
                replied = *delivered = true;
                muse_ledger_reply(text);"""),
    ("""            case MUSE_HATCH_EV_DONE:
                done = *delivered = true;
                break;""",
     """            case MUSE_HATCH_EV_DONE:
                done = *delivered = true;
                muse_ledger_turn_end(true, NULL);
                break;"""),
    ("""            case MUSE_HATCH_EV_ERROR:
                ESP_LOGW(TAG, "muse: %s", text);""",
     """            case MUSE_HATCH_EV_ERROR:
                muse_ledger_turn_end(false, text);
                ESP_LOGW(TAG, "muse: %s", text);"""),
    ("""            muse_hatch_turn_cancel();
            muse_state_set_level(0);
            return true;""",
     """            muse_hatch_turn_cancel();
            muse_ledger_turn_end(true, "interrupted");
            muse_state_set_level(0);
            return true;"""),
]
for old, new in subs:
    assert old in s, "ledger anchor not found: " + old[:60]
    s = s.replace(old, new, 1)
open(p, 'w').write(s)
print("muse_voice.c patched (turn-ledger hooks)")
EOF
else
  echo "muse_voice.c already patched"
fi

echo "== muse_settings_ui.c (vitals + diary pages) =="
SUI="$SDK/components/muse/muse_settings_ui.c"
if ! grep -q "muse_vitals_page_build" "$SUI"; then
  python3 - "$SUI" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
# 1. declarations (board-guarded: the pages only exist on our board)
anchor = "static lv_obj_t *s_tile;"
decls = """#if CONFIG_MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154
void muse_vitals_page_build(lv_obj_t *list);
void muse_vitals_page_tick(void);
void muse_diary_page_build(lv_obj_t *list);
void muse_diary_page_tick(void);
#endif
"""
assert anchor in s, "settings_ui decl anchor not found"
s = s.replace(anchor, decls + anchor, 1)
# 2. page object statics
old = "static lv_obj_t *s_home, *s_wifi, *s_hatch, *s_ble, *s_sound, *s_sleep, *s_battery, *s_power, *s_text;"
new = ("static lv_obj_t *s_home, *s_wifi, *s_hatch, *s_ble, *s_sound, *s_sleep, *s_battery, *s_power, *s_text;\n"
       "#if CONFIG_MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154\n"
       "static lv_obj_t *s_vitals, *s_diary;\n"
       "#endif")
assert old in s, "settings_ui statics anchor not found"
s = s.replace(old, new, 1)
# 3. page shells + page_t entries, next to the other page_t definitions
old = "static const page_t POWER = { &s_power, build_power_page };"
new = """static const page_t POWER = { &s_power, build_power_page };
#if CONFIG_MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154
static void build_vitals_shell(lv_obj_t *tile)
{
    lv_obj_t *list;
    s_vitals = page(tile, "VITALS", true, &list);
    muse_vitals_page_build(list);
}
static void build_diary_shell(lv_obj_t *tile)
{
    lv_obj_t *list;
    s_diary = page(tile, "DIARY", true, &list);
    muse_diary_page_build(list);
}
static const page_t VITALS = { &s_vitals, build_vitals_shell };
static const page_t DIARY = { &s_diary, build_diary_shell };
#endif"""
assert old in s, "settings_ui page_t anchor not found"
s = s.replace(old, new, 1)
# 4. home rows
old = '    row(list, LV_SYMBOL_BATTERY_FULL, "Battery", &s_home_battery, on_nav, (void *)&BATTERY);'
new = """    row(list, LV_SYMBOL_BATTERY_FULL, "Battery", &s_home_battery, on_nav, (void *)&BATTERY);
#if CONFIG_MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154
    row(list, LV_SYMBOL_EYE_OPEN, "Vitals", NULL, on_nav, (void *)&VITALS);
    row(list, LV_SYMBOL_FILE, "Diary", NULL, on_nav, (void *)&DIARY);
#endif"""
assert old in s, "settings_ui home anchor not found"
s = s.replace(old, new, 1)
# 5. tick dispatch
old = """    } else if (s_current == s_battery) {
        tick_battery();
    }
}"""
new = """    } else if (s_current == s_battery) {
        tick_battery();
#if CONFIG_MUSE_BOARD_SMKTEC_S3_TOUCH_LCD_154
    } else if (s_current == s_vitals) {
        muse_vitals_page_tick();
    } else if (s_current == s_diary) {
        muse_diary_page_tick();
#endif
    }
}"""
assert old in s, "settings_ui tick anchor not found"
s = s.replace(old, new, 1)
open(p, 'w').write(s)
print("muse_settings_ui.c patched (vitals + diary pages)")
EOF
else
  echo "muse_settings_ui.c already patched"
fi
