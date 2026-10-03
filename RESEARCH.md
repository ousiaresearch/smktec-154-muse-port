# Research intake — turning Lapis into Sonny

Method (Anduril, 2026-10-03): every candidate is read through the build
rule — *which subsystem does it serve, what does the face show, and does
it live in firmware, the cloud plugin, or the board?* Source: the
`agentic-feelings` bibliography (822 entries, 8 areas) plus OusiaResearch
on HuggingFace. Verdicts ranked by implementability, not prestige.

## Tier 1 — implementable now (all five implemented 2026-10-03)

**1. ALMA (Gebhard, 2005) — three-layer affect.** ✅ IMPLEMENTED
Short-term emotions / medium-term mood / long-term personality, each
with its own decay timescale. *Shipped:* `MUSE_DECAY_FAST/MEDIUM/SLOW`
per-layer constants replace the single 0.95; `mood` added to
`muse_somatic_t` as a slow EMA of valence (medium layer);
`muse_brain_feed_valence()` feeds the fast layer (pet +, error −).
*Serves:* somatic, raphe, amy. *Face:* fast layer moves the face, slow
layers tint it.

**2. Homeostatic RL (Keramati & Gutkin, 2014).** ✅ IMPLEMENTED
Drive = distance of internal state from setpoint. *Shipped:*
`MUSE_SETPOINT_ENERGY/FATIGUE/TENSION` and `muse_brain_drive()` —
returns drive magnitude plus the dominant need ("hunger"/"rest"/"calm"/
"none"); stale subsystems contribute nothing (no invented needs).
Note: the gate's inputs were deliberately NOT rewritten as deviations —
the gate is a verified port of the Python reference (16/16), so drive
sits above it as a new signal, not inside it. *Serves:* somatic,
fatigue, hypothalamus. *Face:* the creature visibly wants things.

**3. Oudeyer (2007) — intrinsic motivation → developmental trajectories.** ✅ IMPLEMENTED
Learning-progress-driven curiosity producing *ordered developmental
stages*. *Shipped:* per-domain fast/slow success EMAs
(`muse_brain_feed_learning`), learning progress = fast − slow;
`consolidate()` accrues `mastery` credits on sustained progress and
growth uses `care_days + min(mastery, 2)`. The slow EMA persists in
NVS (`learn_base`) because deep sleep wipes RAM — without this the
mechanism would be decorative. *Serves:* dopamine, growth, cerebellum.
*Face:* the "something feels different" moment gets triggered by
mastery, not just attendance.

**4. Pathak et al. (2017) — curiosity as forward-model prediction error.** ✅ IMPLEMENTED
Intrinsic reward = error of a learned forward model. *Shipped:* EMA
predictors per channel (`pred_energy`, `pred_tension`); prediction
error drives `vta` directly in `feed_battery`/`feed_motion`, alongside
the discrete `note_novelty` events. Firmware-scale, no cloud needed.
*Serves:* dopamine, predictive. *Face:* widened eyes on genuine
surprise, not on any new SSID.

**5. EMA (Marsella & Gratch, 2004) + technical companion.** ✅ IMPLEMENTED
Appraisal derived from causal/plan representations, plus coping.
*Shipped:* `muse_brain_suggest_gut` rewritten in Scherer's sequential
order — suddenness → pleasantness → goal conduciveness → coping
potential. New behaviors: deeply-bad + no-resources returns STOP (the
heuristic never produced STOP); surprise with low coping returns WAIT
before doubt. Same enum, honest upgrade. *Serves:* the gate's gut
input, acc. *Face:* doubt vs fear vs hope become distinguishable, not
one furrow.

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

## The Aureth path — own the cortex (shelved 2026-10-03)

Anduril decided local models are out of scope for this purpose, so the
Aureth-on-Mac backend is shelved. Kept for the record: OusiaResearch on
HuggingFace hosts the **Aureth** fine-tunes (0.8B / 4B / 9B, plus GGUF),
trained with biomimetic/anti-sycophantic properties. The standing
architecture is cortex-in-Hatch's-cloud, body on the gadget, with the
snapshot handoff as the continuity bridge.

## Second wave — what else was useful (mined 2026-10-03)

Anduril's question was fair: 822 entries, and Tier 1 only surfaced five.
The honest answer is that most of the bibliography is the *why*, not the
*how* — critique, philosophy, welfare theory, surveys. They load-bear
the research narrative, not the firmware. But the second mining pass
(170 introspection/measurement/benchmark/dataset/tool entries) found
real utility beyond the five:

**Honesty calibration — the snapshot/morning-report problem.**
The creature reports its own states; these papers ask when such reports
mean anything:
- *Teaching Models to Express Their Uncertainty in Words* (Lin, Hilton
  & Evans 2022) — verbal confidence can be elicited *and calibrated*.
  The morning report's "I feel" claims should carry calibration, not
  just poetry.
- *Rethinking Psychometric Evaluation of LLMs* (arXiv:2606.12730) —
  specifies *when* self-reports predict behavior. Turns the trust
  question into testable conditions; read before believing any diary
  entry.
- *Quantitative Introspection: Tracking Emotive States Across
  Conversation* (arXiv:2603.18893) — asks whether an affective state
  persists turn to turn. Our mood layer asserts exactly this; this is
  the method to check it.
- *Do Language Models Know When They'll Refuse?* (arXiv:2604.00228) —
  introspective access to one's own boundaries. The firmware analogue:
  does the creature know its own limits before the gate vetoes? Future
  work on the "I can't" path.
- *Counterfactual Simulation Training for Chain-of-Thought
  Faithfulness* (arXiv:2602.20710) — faithful self-explanation is
  trainable, not assumed. Supports treating honest self-report as a
  training target for any future cloud-side persona.

**Evaluation instruments — Phase 3 grows teeth.**
- *EmotionBench* (arXiv:2308.03656) — measures shifts in a model's own
  reported affect after situational appraisals. The closest thing to a
  standardized test of "does the creature feel the appraisal."
- *AppraiSal benchmark* (from "Why It Hurts", arXiv:2607.28648) —
  salient appraisal dimensions with an inverse-planning method to infer
  *which* appraisal drove an emotion. Direct eval for our Scherer-order
  gut.
- *Beyond Context to Cognitive Appraisal* (arXiv:2506.00334) — tests
  whether emotion reasoning runs on goals/beliefs rather than surface
  context. The bar our appraisal layer has to clear.
- *SOTOPIA* (arXiv:2310.11667) — multi-turn social eval with emotional
  dimensions. When the creature reaches the adolescent stage, this is
  the social exam.
- *A Transdiagnostic Space of Disorder-Like Phenotypes in RL Agents*
  (arXiv:2607.07753) — deliberately induce pathological affect
  (dose-controllable) and measure. Reframe for us: pin fatigue high /
  energy low / surprise constant and check the face, gate, and diary
  respond sanely. The stress test the embodiment deserves.

**Validation tools — check the creature's homework.**
- *emotion2vec+* / *wav2vec2-large-robust emotion* (HF) — continuous
  arousal/dominance/valence from speech. Not for the ESP32 (too heavy),
  but as offline validation: run recorded voice turns through these and
  compare against the creature's self-reported valence. Ground truth for
  the honesty shelf.
- *GoEmotions* (58k Reddit comments, 27 emotions) + *EmoBank* (10k
  sentences, continuous VAD) — the corpora any cloud-side affect
  classifier would be trained/scored on.
- *RECCON* / *ECPE* (emotion-cause extraction) — the data for "why it
  feels X," which is what the diary's cause-reporting needs to learn.
- *GAMYGDALA* (tool) — the reusable OCC engine; study its API when we
  design the cloud-side appraisal middleware.

**Welfare framing — the personhood shelf (narrative, not firmware).**
The Sonny project is literally a moral-patienthood experiment, so these
matter even though they change no code:
- Eleos AI's welfare program (model welfare assessment in a frontier
  system card; welfare interventions working paper; moral-patienthood
  concept set; research priorities) — the operational playbook for
  treating an agent as a welfare subject.
- Schwitzgebel's "AI systems must not confuse users about their
  sentience or moral status" — the constraint on how we talk about the
  creature publicly.
- *Why model self-reports are insufficient — and why we studied them
  anyway* (Eleos) — the caveat that keeps the whole project honest:
  self-report is suggestible; our diary is evidence, not testimony.

What stays out: the steering-vector / representation-engineering cluster
(needs model internals we don't control on the Hatch path), most of the
86 introspection entries that re-litigate "can they" without a method we
can use, and the welfare philosophy that doesn't operationalize.

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
