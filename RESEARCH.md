# Research intake — turning Lapis into Sonny

Method (Anduril, 2026-10-03): every candidate is read through the build
rule — *which subsystem does it serve, what does the face show, and does
it live in firmware, the cloud plugin, or the board?* Source: the
`agentic-feelings` bibliography (822 entries, 8 areas) plus OusiaResearch
on HuggingFace. Verdicts ranked by implementability, not prestige.

## Tier 1 — implementable now

**1. ALMA (Gebhard, 2005) — three-layer affect.**
Short-term emotions / medium-term mood / long-term personality, each
with its own decay timescale. We have all three layers already (face
state, raphe mood tone, identity/values) but never formalized the
timescales. *Work:* write the timescale table into `muse_brain`
(fast: arousal/tension seconds; medium: mood hours; slow: traits
persistent), per-subsystem decay constants instead of one 0.95.
*Serves:* somatic, raphe, amy. *Face:* fast layer moves the face, slow
layers tint it.

**2. Homeostatic RL (Keramati & Gutkin, 2014).**
Drive = distance of internal state from setpoint; reward = its
reduction. This is the formal version of our somatic subsystem.
*Work:* recast `muse_somatic_t` around setpoints (energy→0.8,
fatigue→0.15, tension→0.2); drive magnitude = deviation vector length;
the gate's `energy`/`fatigue` inputs become deviations, not raw levels.
*Serves:* somatic, fatigue, hypothalamus. *Face:* the creature visibly
wants things (low energy reads as hunger, not a number).

**3. Oudeyer (2007) — intrinsic motivation → developmental trajectories.**
Learning-progress-driven curiosity producing *ordered developmental
stages* in robots. This is the growth-stages paper: our care-day
thresholds are a placeholder for what should be learning-progress
milestones. *Work:* track prediction accuracy per domain (gesture
recognition confidence, voice-turn success); stage advancement weighs
learning progress alongside care-days. *Serves:* dopamine, growth,
cerebellum. *Face:* the "something feels different" moment gets
triggered by mastery, not just attendance.

**4. Pathak et al. (2017) — curiosity as forward-model prediction error.**
Intrinsic reward = error of a learned forward model. Upgrades our
novelty heuristic (`+0.3` on novelty events) to a real signal: the vta
drive becomes *surprise at the world model*. *Work:* tiny predictor
(next sensor reading / next interaction outcome); prediction error
feeds `vta`. Firmware-light; the model can live cloud-side with the
scalar on-device. *Serves:* dopamine, predictive. *Face:* widened eyes
on genuine surprise, not on any new SSID.

**5. EMA (Marsella & Gratch, 2004) + technical companion.**
Appraisal derived from causal/plan representations, plus coping. The
companion paper is explicitly "what agent builders actually port."
*Work:* upgrade `muse_brain_suggest_gut` from the 3-branch heuristic to
appraisal checks in Scherer's order (novelty → pleasantness → goal
relevance → coping potential). *Serves:* the gate's gut input, acc.
*Face:* doubt vs fear vs hope become distinguishable, not one furrow.

## Tier 2 — design guidance (read, don't port yet)

- **WASABI (2010):** dimensional core affect + OCC secondary emotions.
  Validates our split: somatic valence/arousal IS the core; the gate's
  gut is the secondary layer. No work — confirmation.
- **Cathexis (1997):** emotions as proto-specialists with elicitation
  and decay dynamics. Our subsystems already rhyme; deepen individual
  time constants per ALMA.
- **FLAME (2000):** appraisal coupled to expectation learning. The
  formal version of our "adaptive thresholds" item.
- **GAMYGDALA (2014, tool):** reusable OCC engine with a clean API.
  Study its API shape when designing the gate middleware.
- **Kismet (Breazeal, 2003):** emotion as internal regulation that
  *also* shapes interaction. The canonical "affect is functional, not
  performed" — our tell-don't-show principle, peer-reviewed.
- **Deeply Felt Affect (2021):** valence = rate of change of free
  energy. Advanced; revisit when formalizing valence dynamics.

## Tier 3 — Phase 3 evaluation

- **Minding Motivation (2025, benchmark):** measures how intrinsic
  motivation formulations change behavior, not just return. The
  comparison our affect claims need.
- **Gubernaut (2026):** deterministic homeostatic controller on LLM
  agents, validated across benchmarks. Closest existing head-to-head
  test of "does affect change behavior."
- The introspection-and-self-report area (155 entries) is the
  methodology shelf for the honesty work: snapshot design, the
  never-lie staleness rule, morning-report filtering.

## The Aureth path — own the cortex

OusiaResearch on HuggingFace hosts the **Aureth** fine-tunes
(0.8B / 4B / 9B, plus GGUF), trained with biomimetic/anti-sycophantic
properties on Hermes data + the Aureth corpus. Two implications:

1. **The cloud half doesn't have to be Meta's cloud.** AurethV2-4B-GGUF
   runs on Anduril's Mac via llama.cpp — the `lapis-embodiment` backend
   (consolidation fitting, ofc/predictive/tom modeling) can run locally
   on his own fine-tune. Full cognitive sovereignty: diary on the card,
   mind on the Mac.
2. **Persona continuity.** If the gadget's turns can ever be routed to a
   chosen model, Aureth is the creature's native voice — trained with
   the same biomimetic priors as the firmware. Until then, the snapshot
   handoff carries the state to whatever serves the turn.

## Area map (for future intake)

- `interoception-homeostasis-neuromodulation-intrinsic-motivation`
  (135) — the drive/curiosity shelf; mined above.
- `introspection-and-self-report` (155) — the honesty shelf.
- `affective-computing-architectures` (97) — the architecture shelf;
  mined above.
- `tools-datasets-benchmarks` (68) — the evaluation shelf.
- `machine-consciousness-sentience-and-ai-welfare` (58) — the
  personhood framing shelf; read for the research narrative, not for
  firmware.
- Remaining areas (`emotion-and-affect-representations-in-llms`,
  `emotion-representations`) — mined on demand.
