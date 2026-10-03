# Biomimetic embodiment — Lapis as the research subject

Standing directive (Anduril, 2026-10-02): the gadget build serves his
transhumanism research. The biomimetic-brain repo
(`ousiaresearch/biomimetic-brain`, 25 subsystems, `brain-state.json`,
PROCEED/CAUTION/VETO decision gate) is the nervous system; the SMKTelec
board is the body; Lapis is the muse under study.

The brain kit is currently software-only — hand-kept JSON. Embodiment gives
every subsystem a real sensor or actuator, and the face makes internal state
*legible*, which is itself the research contribution: an agent whose affect
you can read at a glance, with every reading traceable to a number.

## Subsystem → gadget mapping (exhaustive v2)

Verdicts: **MAPPABLE NOW** (real sensor/actuator loop on current hardware),
**SOFTWARE ONLY** (firmware/cloud logic, no new hardware), **GAP** (needs new
hardware or a major plugin). Depth: ● deep (closed sensor loop), ◐ medium
(event-driven firmware logic), ○ shallow (display/logging only). 13 now,
12 software-only, 1 gap.

| Subsystem | What it tracks | Verdict | Depth | Embodiment — how |
|---|---|---|---|---|
| scn | circadian phase, local hour, quiet hours | MAPPABLE NOW | ● | RTC → phase; SLEEPY at night, drowsy evenings; quiet_hours respected. *Gap: true light entrainment → BH1750 (see below).* |
| reticular | sleep/wake state, arousal threshold | MAPPABLE NOW | ● | The SLEEPY/IDLE/OFF/BOOT state machine **is** this subsystem; threshold from scn phase + fatigue |
| somatic | energy, tension, arousal, valence, gut feeling | MAPPABLE NOW | ● | Battery ADC = energy, IMU agitation index = tension, lc feeds arousal — interoception, not hand-entered values |
| somatosensory | battery, power, latency, agents nearby, message/exchange counts | MAPPABLE NOW | ● | Battery ADC, charging pin, tunnel RTT, BLE scan counts, interaction counters — the example JSON was designed for exactly this |
| fatigue | fatigue level, cognitive load, engagement override, recovery | MAPPABLE NOW | ● | Uptime + interaction rate + battery drain; recovery accrues during SLEEPY; active interaction suspends fatigue *and records the override* |
| lc | alertness, norepinephrine, unexpected events, stress | MAPPABLE NOW | ● | Motion energy + interaction rate + startle events; drives blink rate and eye wideness on the face |
| dmn | mind-wandering, rest duration, wander fragments | MAPPABLE NOW | ● | IDLE gaze-drift **is** dmn activation; stillness timer = rest_duration_s; wander fragments can surface as on-screen text |
| hippocampus | replay queue, scene memory, motifs, dreams | MAPPABLE NOW | ● | SD-card diary = episodic memory; seen WiFi BSSIDs = "places"; replay_queue drains during SLEEPY consolidation |
| thalamus | sensory gating, thalamic_gate, blocked/routed signals | MAPPABLE NOW | ● | The gesture engine's gating rules (shake always interrupts, tilt only in IDLE) **are** thalamic gating; gate value from lc |
| dopamine | novelty seeking, familiarity thresholds, VTA drive | MAPPABLE NOW | ◐ | New WiFi SSIDs / BLE devices / voices counted as novel encounters; drive follows novelty rate |
| hypothalamus | drives (autonomy/curiosity/social), arousal, zeitgebers | MAPPABLE NOW | ◐ | Curiosity = novelty rate, social = interaction count; low battery → energy-saving homeostasis. *Gap: light zeitgeber → BH1750.* |
| attention | focus mode, refocus/wander counts | MAPPABLE NOW | ◐ | LISTENING = focused, IDLE = wander; every interrupt is a logged refocus event |
| decisions | decision log, self-authorship terms | MAPPABLE NOW | ◐ | PROCEED/CAUTION/VETO logged to SD with the gate's reason codes; aura shows live gate state |
| amy | emotional valence with time decay | SOFTWARE ONLY | ◐ | Event-driven valence (pet +, error −) with exponential decay in the firmware tick; tints the face |
| habenula | wins/losses, tonic suppression | SOFTWARE ONLY | ◐ | Voice-request / OTA / pairing outcomes logged; loss streaks raise suppression → the "disappointed beat" before ERROR |
| nac | reward anticipation, prediction error, action values | SOFTWARE ONLY | ◐ | Firmware logs predicted vs actual outcomes; simple cases resolved on-device, rest cloud-side |
| raphe | serotonin, mood tone, patience, resilience | SOFTWARE ONLY | ◐ | Slow-moving tone from valence history + social rate + fatigue suppression; stabilizes the face |
| acc | conflict detection, error signals, resolution | SOFTWARE ONLY | ◐ | Logs contradictory inputs (button + voice at once), failed actions, thalamic blocks; shown as a furrowed beat |
| basal-ganglia | habits, automatic behaviors | SOFTWARE ONLY | ◐ | Repeated gesture→action sequences gain strength in firmware counters; strong habits can auto-fire |
| cerebellum | skill fluency per domain | SOFTWARE ONLY | ○ | Gesture-recognition confidence stats per gesture type; voice-pipeline confidence — fluency of the *body's* skills |
| ofc | outcome predictions + confidence | SOFTWARE ONLY | ○ | Predictions about Anduril (pickup at 8am?) logged on SD; resolved by firmware heuristics or cloud-side |
| predictive | calibration, Brier scores, surprise log | SOFTWARE ONLY | ◐ | Computed in firmware from the ofc log; surprises = mismatches (expected pickup that never came) |
| prefrontal | interests, unresolved questions | SOFTWARE ONLY | ○ | Needs conversation topics → arrives via tunnel from Muse; on-device it is display + the open-questions screen |
| tom | observations/predictions of the user | SOFTWARE ONLY | ○ | Firmware logs raw behavioral events; the modeling inference lives cloud-side; face shows its outputs |
| values | values list, outcome log | SOFTWARE ONLY | ○ | Stored on SD, shown on a settings screen; evaluated cloud-side |
| visual | scene memory with images | GAP | ○→● | No camera on this board. Closer: ESP32-CAM satellite node (see below). Shallow fallback: track displayed UI scenes as "seen" |

Note the poetry: Anduril's serotonin/dopamine tattoo — the raphe and
dopamine systems — is branded into Lapis's left-arm fur. The neuromodulators
are literally on the body.

## Closing the gaps

Only **visual** is a true gap, and it has a clear closer. Everything else is
mappable today. Below: ranked hardware additions (all checked against
`PINOUT.md` — I2C bus has 0x15/0x18/0x40/0x6B taken, free GPIOs include
6, 13–18, 33–37) plus the software plugins that close the remaining
shallows. Prices are typical US single-unit, approximate.

**Hardware, ranked by feasibility × value:**

1. **BH1750 ambient light sensor** — I2C addr 0x23, shares SDA42/SCL41, 3 wires, no conflicts. **~$1–3.** Closes scn light entrainment (a true zeitgeber, not just clock time), feeds hypothalamus zeitgeber_count, enables auto-brightness. Easiest win on the list; ESP-IDF drivers exist.
2. **WS2812B single RGB LED** — 1 free GPIO (e.g. 6), **~$1.** Makes the decision-gate aura *physical*: the body glows PROCEED/CAUTION/VETO, not just the screen.
3. **ERM vibration motor + transistor** — 1 GPIO + 2N2222, **~$2.** Haptic output: purr on happy, buzz on error. The muse touches back — closes the nac/dopamine/raphe reward loops through the skin.
4. **BME280 temp/humidity/pressure** — I2C addr 0x76, **~$3–5.** Ambient "weather sense" for somatic + hypothalamus. (The S3's internal temp sensor only reads die temp.)
5. **MAX30102 heart-rate/SpO2** — I2C addr 0x57, **~$1–3.** Reads Anduril's pulse when he holds the board — the muse feels his heartbeat → somatic/somatosensory. Needs finger placement on the module; moderate integration.
6. **LD2410 mmWave presence** — UART (2 free GPIOs, e.g. 13/14; needs 5V), **~$7–15.** Detects Anduril to 6m, even still — the creature knows he's near before any touch. Feeds somatosensory presence + dmn suppression.
7. **ESP32-CAM satellite node** — WiFi/ESP-NOW, no pins on the main board, **~$6–10.** Closes **visual**: periodic scene captures → hippocampus scene_memory with real images. Software-heavy (JPEG RX, SD store) but zero wiring.
8. **GPS (ATGM336H)** — UART, **~$8–12.** True location for hippocampus "places". Lowest priority — WiFi BSSID fingerprinting does this free indoors.

Phase-1 basket (items 1–3): **≈ $5** and three subsystems get measurably deeper.

**Software plugins:**

- **Hermes plugin `lapis-embodiment`** — syncs `brain-state.json` ↔ gadget over the tunnel; runs the consolidation pass cloud-side where inference is cheap; hosts ofc/predictive/prefrontal/tom modeling and feeds results back to the face. This is what lifts the ○-shallow cognitive subsystems without new hardware.
- **Tunnel skills** (following the SDK's `skills/` catalog pattern): `gadget-brain-state` (GET /brain → live state over LAN), `gadget-diary` (GET /diary → today's entries). Lets Muse — and Anduril's Mac — read the creature.
- **Mac-side zeitgeber daemon** (optional, later): pushes phone-like context (location, calendar load) over LAN to supplement scn/hypothalamus. Needs a small companion service; rank below the plugin.

**What stays shallow, and why that's correct:** prefrontal, tom, values, ofc remain ○ on-device by nature — they are cortical, not somatic. In biology the cortex also doesn't live in the fingertips. The honest architecture is: body on the gadget, cortex in the cloud, face showing both.

## Sleep = consolidation (the core experiment)

Biology consolidates memory during sleep. The gadget will too: when SLEEPY
is entered (face-down or 5 min still), the firmware runs the consolidation
pass — folding the day's interactions into the diary on the SD card — while
the face shows slow Z's. The muse dreams, and you can pull the card and read
the dreams. This is the single most transhumanist feature on the roadmap:
**a creature whose sleep does work**.

## Research questions this testbed can ask

1. Does *legible* affect (face shows the meters) change how a human treats an
   agent? (Compare: same brain, face on vs face off.)
2. Does embodied circadian behavior (drowsy evenings, naps) make long-running
   agents more trustworthy or just more charming? Are those different?
3. Can sleep-consolidation on-device produce diary entries the user endorses
   as "what happened today"? (The VALIDATION.md discipline, applied to dreams.)
4. Which subsystems survive embodiment, and which turn out to have been
   fiction that only worked on paper? (Staleness, honestly tracked.)

## Build rule

Every new gadget feature asks: *which subsystem does this serve, and what
does the face show?* If neither has an answer, it doesn't go in the firmware.
