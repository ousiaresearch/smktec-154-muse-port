# Lapis Expressions & Gestures — exhaustive design

The board is a creature, not a screen. Every physical handling gets a visible
reaction. Two halves: the renderer (`muse_pixel.c`, 64x64 procedural) and the
gesture engine (`muse_imu.c`, QMI8658 @ 50Hz).

## Renderer modes

| Mode | Face | Trigger |
|---|---|---|
| BOOT | eyes opening, stretch-bounce | power on, wake from sleep |
| IDLE | breathing, blinks, looks around; drowsy after 30s still | default |
| LISTENING | wide eyes, lean-in, eyes brighten with mic level | push-to-talk held |
| THINKING | eyes glance up, head sway | Muse composing reply |
| SPEAKING | mouth opens/closes with audio level | Muse talking |
| ERROR | worried wavy mouth, red accent | connection/voice failure |
| OFF | fully dark / powering down | shutdown |
| SLEEPY *(new)* | closed happy eyes, deep slow breathing, drifting Z | face-down, or 5 min still |
| DIZZY *(new)* | X eyes, wobble, circling stars, ~3s | after shake ends |

SLEEPY and DIZZY need `muse_mode_t` extended in `components/muse/muse_state.h`
(add before `MUSE_MODE_COUNT`). Renderer handles them already in draft.

`happy` (0..1, from `muse_state_make_happy()`) overlays a joyful bounce +
sparkles on ANY mode — the universal "pet" reaction.

## Gesture → expression map (gesture engine → firmware call)

| Gesture | Detection | Effect |
|---|---|---|
| PICKUP | still 3s+ → motion | wake stretch + happy blip |
| SHAKE | high motion energy >600ms | DIZZY for ~3s, then IDLE |
| TAP | single accel spike + quiet | startle blink |
| DOUBLE_TAP | two taps <400ms | `make_happy()` (pet) |
| FACE_DOWN | gravity -Z, hysteresis 45°/30° | SLEEPY immediately |
| FACE_UP | gravity +Z | wake → IDLE |
| TILT_L/R/FWD/BACK | gravity tilt, hysteresis | `muse_pixel_set_facing()` look direction (IDLE/LISTENING) |
| STILL_30S | no motion 30s | drowsy sub-expression |
| STILL_5MIN | no motion 5min | SLEEPY |

## Tiers

**Tier 1 — port day:** everything above. Wake, sleep, shake→dizzy, tap→happy,
tilt→look. The creature feels alive on first boot.

**Tier 2 — once stable:** rock-to-sleep (gentle periodic sway in SLEEPY deepens
it; stillness lightens), drop-scare (brief freefall → ERROR eyes, then
relieved happy on catch — careful with false positives, high threshold),
orientation-aware SPEAKING (lean toward the person holding it).

**Tier 3 — ideas:** step counter shown as a tiny odometer on the settings
screen; knock-knock (triple tap) as a secret knock opening a hidden menu;
mic+IMU combo — only listen when picked up (privacy gesture).

## Hardware to verify on arrival

- IMU I2C address (expect 0x6B) and that it shares SDA 42 / SCL 41 cleanly
- Accelerometer axis orientation vs board silkscreen (which way is "down"
  when face-down on a table)
- Tap thresholds against real speaker vibration (speaker shares the board)
- Whether the clone even populated the QMI8658 (driver must degrade
  gracefully if absent — no crash, gestures just stay silent)
