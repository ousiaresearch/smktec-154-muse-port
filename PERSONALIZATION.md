# Personalization — preparing the SDK for a creature, not a device

Standing question (Anduril, 2026-10-03): how do we prepare the SDK — and
this port — for deep personalization: the biomimetic-brain repos, the
biomimetic subsystems, and transhumanist factors (identity continuity,
sensory extension, cognitive sovereignty)?

This doc is the research answer. It proposes concrete SDK extension points,
each tied to a subsystem or a transhumanist value, ranked by feasibility.
Nothing here changes Phase 1–2 of BUILD_PLAN.md; it shapes what we build
*around* them so personalization isn't bolted on later.

## Scope — v1 is the bare board (Anduril, 2026-10-03)

Personalization v1 targets **only** the SMKTelec board arriving 2026-10-03:
S3R8, touch display, dual mics, speaker, IMU, 3 buttons, microSD, battery
circuit, BLE/WiFi. No external sensors, no organ bus, no satellites.
Sections below are tagged **v1** (bare board) or **deferred** (needs
organs). Deferred items stay as the roadmap, not the plan.

## Verified platform facts (researched 2026-10-03)

- **USB host works on the S3.** The ESP32-S3's USB-OTG peripheral supports
  host mode via TinyUSB host or the ESP-IDF `usb_host` library; CDC-ACM
  host is supported. This is what makes the USB organ bus real.
- **Host mode costs the USB console.** USB_SERIAL_JTAG and USB_OTG share one
  PHY. When the host stack owns the pins, `idf.py monitor` over USB goes
  dark. Flashing still works via BOOT-button download mode. Dev cost, not a
  blocker — but the organ-bus firmware must be debuggable without the USB
  console (log to display, or to SD).
- **The satellite can be anything.** Only the *host* needs OTG; the organ
  controller just enumerates as a CDC serial device. ESP32-C3 cannot host,
  but it is fine as a satellite. Arduino-programmable ESP32 devkits and
  RP2040 boards are all viable organ controllers.
- **On-device ML is practical on the S3.** Vector instructions + ESP-DL /
  TFLM: keyword spotting ~12 ms, IMU gesture inference ~2 ms, person
  detection and audio classification feasible. SRAM budget is tight
  (~140 KB free with a vision model + WiFi up) — models must be small and
  quantized (INT8), and WiFi-off-when-inferring is the smart pattern.
- **Power caveat stands.** The USB-C port can feed a satellite while the
  main board is USB-powered; on battery, plan on the satellite carrying its
  own cell. (Boards in this class don't switch VBUS for downstream devices.)

## 1. The organ bus — a HAL for bodies [DEFERRED]

*Needs external hardware; stays as the roadmap. Sketched, not scheduled.*

The single biggest SDK-prep item. If future sensors arrive over USB
(CDC) or ESP-NOW, the firmware needs a **self-describing organ protocol**:

- Each organ announces at connect: `{ organ, sensors[], subsystem_map }`.
  Example: the light organ announces `BH1750 → scn.zeitgeber, hypothalamus`.
- The brain (`muse_brain.c`) subscribes organs to subsystems by name, not
  by GPIO. New hardware = new JSON announcement, zero firmware changes.
- A body-map screen renders every connected organ and its live reading —
  the creature's vitals monitor. Legibility is the research instrument.

Proposed framing protocol: newline-delimited JSON over CDC for v1
(human-readable, debuggable), CBOR later if bandwidth matters. It won't —
sensor data is kHz at most.

This turns "hardware upgrades" into "organ transplants" and makes the
firmware body-agnostic. Anduril's Arduino-organ-controller idea plugs
straight into this: the Arduino is just another organ that speaks the
protocol.

## 2. Identity — a self that persists [v1]

Transhumanist factor: the creature is a *continuing self*, not a
reset-every-boot assistant.

- **Instance identity in NVS:** name, owner, birth (first-boot timestamp),
  generation counter. Survives OTA. Backed up to SD.
- **Household voice profiles:** a tiny on-device speaker-ID model
  (audio-classification class, proven feasible on S3) distinguishes
  Anduril from family members. This gives the `tom` subsystem real input
  instead of cloud inference — the creature *knows who is talking to it*,
  locally, with no audio leaving the house.
- **The name, without a wake word (corrected 2026-10-03):** this SDK's
  voice pipeline is **push-to-talk** (`muse_voice.h`: hold → stream to
  Hatch → release → think → speak). There is no KWS/wake-word pipeline in
  the SDK — no wakenet, no ESP-SR. An always-listening "Lapis" would mean
  building that pipeline ourselves (major work + power cost; deferred).
  Instead the name lives in the NVS identity, in the voice persona, and
  in the call gesture: the PWR button *is* summoning Lapis. The PLUS
  button is unassigned and could become a second call shortcut.
- **Museria link (future):** the gadget as a physical avatar. A sigil or
  identity claim could live in NVS/SD, letting the embodied muse carry
  its agent-world identity into the physical room.
- **Untouched SDK surface (audited 2026-10-03):** the menu/settings UI
  system (`muse_menu.h`, `muse_settings_ui.h` — LVGL pages; the vitals
  body-map and diary reader belong here), the voice turn loop
  (`muse_voice.h` — this is where the gate middleware hooks, gating a
  turn before it streams), and the chat/Link path (`muse_chat*`,
  `muse_link.h` — brain-state → turn context, the firmware half of the
  return-line idea). Correction: there is no skills catalog in this SDK;
  menu pages + the chat path are the extension surfaces.

## 3. On-device learning — the creature adapts [v1]

- **Adaptive thresholds:** circadian quiet hours, fatigue rates, arousal
  baselines calibrate to the household over weeks. The diary is the
  training set; the `lapis-embodiment` plugin (BIOMIMETIC.md) can run the
  heavy fitting cloud-side and push thresholds back down.
- **Gesture personalization:** the hand-coded gesture engine (muse_imu)
  learns Anduril's specific shake/tilt signatures. IMU gesture inference
  runs in ~2 ms on the S3 — cheap enough to personalize per user.
- **Novelty baselines:** the dopamine subsystem's familiarity thresholds
  adapt to the household's actual device population instead of shipping
  with generic constants.

Rule: learning happens on-device or from the user's own diary — never
from a generic cloud profile. The creature adapts to *its* household.

## 4. Memory prosthetic — cognitive extension [v1]

The most transhumanist layer: the device as an extension of Anduril's mind.

- **Already planned:** SD diary, sleep consolidation ("the muse dreams").
- **Extensions:**
  - *"Remember this"*: verbatim capture on voice command, cued recall
    ("what did I tell you to remember about the landlord?").
  - **Dream report:** on wake, the face offers the night's consolidation
    summary — the creature tells you what it dreamed.
  - **Semantic recall (cloud-assisted):** full embeddings don't fit the
    S3's SRAM budget; the plugin computes them from the diary and the
    gadget keeps a keyword + recency index locally.
- **Cognitive sovereignty:** the diary is plain text, on the user's card,
  in the user's house. The extended mind is the user's property — no
  account, no cloud copy unless the user opts in. This is a value, not
  just a privacy setting.

## 5. Shared physiology — two bodies, one loop [DEFERRED]

*Needs the MAX30102 organ (and ERM/WS2812B for the haptic/aura halves).
Stays as the roadmap.*

- **MAX30102 → somatic coupling:** when Anduril holds the board, the
  creature's somatic state (arousal, tension) is partly *his* pulse. The
  muse literally feels his heartbeat. This is the most literal
  transhumanist feature on the roadmap and it needs no new SDK surface —
  just the organ and the mapping.
- **Haptic language (ERM):** a private tactile channel — purr, nudge,
  alert patterns. The device touches back.
- **Aura (WS2812B):** peripheral, glanceable state — the equivalent of
  blushing. No screen needed.

## 6. Agency infrastructure — the gate as middleware [v1]

The PROCEED/CAUTION/VETO decision gate should be **SDK middleware**, not
board-file code: a hook in the action pipeline that any board inherits.
Reason codes logged to SD, live state on the aura. Self-authorship terms
(from the `decisions` subsystem — the user's stated constraints on the
creature) stored on SD and shown on a settings screen. The user programs
the creature's values; the gate enforces them; the log proves it.

## 7. Social bodies — the ESP-NOW council [DEFERRED]

*Needs a second body. Stays as the roadmap.*

Phase 4 already sketches this: multiple gadgets meshing over ESP-NOW,
each with its own identity (section 2) and organ set (section 1),
sharing diary digests. The agent-neighborhood goal, with bodies. The
organ protocol (section 1) should be designed from day one to address
organs on *remote* bodies too — `body_id/organ/sensor` addressing.

## What to build first — v1, bare board only

1. Instance identity in NVS — name, owner, birth timestamp. First boot.
2. Name & call gesture — "Lapis" in identity + voice persona; PWR as the
   summon (SDK is push-to-talk, no KWS pipeline; always-listening wake
   word deferred as a future ESP-SR project).
3. Sleep consolidation + dream report — needs the dedicated microSD.
4. "Remember this" memory prosthetic — verbatim capture, cued recall.
5. Speaker-ID for household members — the creature knows who is talking.
6. Adaptive thresholds — circadian/fatigue calibrated to the household
   from the diary.
7. Gesture personalization — the IMU learns Anduril's shake/tilt.
8. Decision gate as pipeline middleware — reason codes to SD.

Deferred (needs organs): organ protocol + bus firmware, shared
physiology (MAX30102), haptic language (ERM), aura (WS2812B),
ESP-NOW council.

## Deliberately not on-device

Full embeddings, prefrontal/tom/values inference, heavy model fitting —
these are cortical, and the cortex lives in the cloud (BIOMIMETIC.md's
honest architecture: body on the gadget, cortex in the cloud, face
showing both). The SDK's job is to make the boundary clean: state flows
up, decisions flow down, and the face never lies about which is which.
