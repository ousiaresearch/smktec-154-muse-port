# Build plan — embodied biomimetic muse

**Goal:** an ESP32-S3 gadget that is a *body* for the biomimetic-brain
architecture, with Lapis as the research subject. Every subsystem gets a
sensor or actuator; the face makes internal state legible; sleep does
memory consolidation. See `BIOMIMETIC.md` for the subsystem map.

**Owners:** Lapis does firmware, research scaffolding, and docs. Anduril does
everything physical: the Mac, the board, the SDK token, his eyes on hardware.

---

## Phase 0 — Groundwork (done, 2026-10-02)

- [x] Board driver draft, API-verified (ES7210 dual-mic fix, CST816S macro)
- [x] Avatar renderer v1 — 9 modes, GIF-verified, tattoos in place
- [x] IMU gesture engine draft (QMI8658, 11 gestures)
- [x] `apply.sh` v2 (SDK-root arg, idempotent, tested)
- [x] Repo live: `ousiaresearch/smktec-154-muse-port`
- [x] Mac setup guide, porting checklist, expression map, biomimetic map

## Phase 1 — Bring-up day (board arrival, on Anduril's Mac)

1. **Mac setup** (Anduril, ~1h): ESP-IDF v6.0.1, SDK clone, port repo clone,
   `./apply.sh`, SDK token from gadgets.muse.ai. Guide: `MAC_SETUP.md`.
2. **Hardware verify** (Anduril, 15 min): USB VID/PID → `ports.py`; IMU
   address (expect 0x6B) and axis orientation; button layout vs silkscreen;
   touch orientation flags. Report values to Lapis.
3. **Build** (`tools/muse/board.sh build 154`): resolve remaining VERIFY
   items (touch mirror flags, ES7210 I2C address, qmi8658 header names at
   build time). Lapis fixes drafts, Anduril rebuilds.
4. **Back up factory firmware** (16 MB read-flash) — before any write.
5. **Flash, boot log**: expect `Muse Gadget starting`, PSRAM found, BLE
   advertising as `MuseGadget-XXXXXX`.
6. **Feature bring-up, in order**: display → backlight → touch → buttons →
   speaker → mics (stereo check) → IMU gestures → BLE pairing → Wi-Fi →
   tunnel → OTA.
7. **Wire the IMU in**: `muse_imu_init(s_i2c)` + gesture→mode mapping in the
   board file; extend `muse_mode_t` with SLEEPY/DIZZY; add
   `muse_pixel_set_facing()` prototype. (See INTEGRATION.md §9.)
8. **Face check**: my nine modes on the real 240×240 panel — especially
   tattoo legibility. Second-pass art if needed.

## Phase 2 — Embodiment v1 (firmware, after bring-up is stable)

The biomimetic layer. All hardware-independent except sensor hooks —
scaffolding can start before Phase 1 finishes.

1. **`muse_brain.c` — brain-state on device.** Port the brain-kit concept:
   one struct per subsystem, folded into a single state snapshot, persisted
   to SD (full) / NVS (essentials). v1 subsystems: `scn`, `somatic`,
   `fatigue`, `lc` (arousal), `dmn`, `hippocampus`, `decisions`.
   Staleness rule from the kit: a quiet subsystem reads `stale`, never lies.
2. **Sensor → subsystem wiring**: battery ADC → somatic/fatigue;
   IMU motion → somatic/lc; wall clock → scn; interaction recency →
   attention/lc; voice events → dopamine (reward).
3. **Face reads the state**: renderer modulates from brain-state, not just
   app mode — high fatigue → drowsy overlay; low arousal → slow blinks;
   novel stimulus → curious tilt-look.
4. **Decision gate → aura**: PROCEED/CAUTION/VETO as glow color, shown
   honestly on the face.
5. **Sleep = consolidation**: entering SLEEPY runs the consolidation pass
   and appends the diary entry to SD. The muse dreams; the card holds them.
6. **State on screen**: a settings-screen page showing the health summary
   (the return-line idea, made visible).

## Phase 2b — Personalization v1 (bare board only)

Scoped 2026-10-03: no external sensors, no organ bus. The board's own
senses — ears, voice, touch, balance, memory, face — are enough for a
creature. Spec: `PERSONALIZATION.md`.

| # | Item | Module | Status |
|---|---|---|---|
| 1 | Instance identity (NVS: name, owner, birth, boot count, generation) | `muse_identity.c` | drafted, needs Mac build |
| 2 | Decision gate as middleware (26 rules, PROCEED/CAUTION/VETO + reason) | `muse_gate.c` | done, host-tested 16/16, 10/10 parity vs Python |
| 3 | Sleep consolidation + dream report | `muse_brain.c` + SD diary | scaffold after Phase 2 |
| 4 | "Remember this" memory prosthetic | `muse_brain.c` + voice cmd | scaffold after Phase 2 |
| 5 | Personal wake word "Lapis" | ESP-SR / KWS pipeline | after bring-up (needs mics) |
| 6 | Speaker-ID (household members) | on-device audio classifier | after wake word works |
| 7 | Adaptive thresholds from the diary | `muse_brain.c` | after consolidation lands |
| 8 | Gesture personalization (IMU) | `muse_imu.c` extension | after bring-up |

Wire-in order on the Mac: identity init first (every boot mints or
loads the self), gate available to the board file as soon as
`muse_brain.c` feeds it readings. Face shows gate state from day one.

## Phase 3 — Research instrument

1. **Time-series logging** of brain-state to SD — the dataset.
2. **Experiments** from BIOMIMETIC.md's four questions (legibility,
   circadian trust, dream endorsement, fiction detection).
3. **Validation discipline** ported from the kit's VALIDATION.md: prove the
   meters move the face, and the face moves the human.

## Phase 4 — Growth

The exhaustive extension map (tunnel skills, diary reader UI, ESP-NOW
council, custom screens, Linux-side commands). Picked per research value,
not novelty.

---

## Decisions (answered by Anduril, 2026-10-02)

1. **SDK token** — provided in chat. Goes into `build-154/sdkconfig`
   (`CONFIG_GADGET_SDK_TOKEN`) on the Mac before first build. Never committed.
2. **Subsystem scope** — map as many of the 25 as possible; for unmappable
   ones, research concrete hardware additions or plugins to close the gap.
   (See BIOMIMETIC.md v2 mapping, in progress.)
3. **Diary privacy** — plain text on the SD card. Anduril will use a
   dedicated microSD (not his Switch card — don't share cards between
   devices; any card works after FAT32 format, but keep one for the muse).
4. **Upstream** — yes: contribute the board port back to the SDK repo once
   stable. The avatar renderer stays ours.
