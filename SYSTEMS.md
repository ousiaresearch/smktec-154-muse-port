# SYSTEMS.md — the meshed architecture: from the 822's "why" to the "how"

This is the synthesis document. Twenty-five mechanism-bearing papers were
read in full (briefs in `DEEPREAD.md`, each with Mechanism / Formal core /
Firmware portability / Design answers / Access). This doc meshes them into
one coherent architecture for the creature: modules, interfaces, update
rules, firmware cost, and what lives where. Equations are quoted from the
briefs, which cite their sources; nothing here is invented math.

The eight open design questions it answers: Q1 drive aggregation · Q2
formal valence · Q3 boredom · Q4 moving setpoints · Q5 learning-rate gating ·
Q6 body→action beyond the veto · Q7 what persists across sleep · Q8 social.

---

## Design principles (distilled, one line each)

1. **Drive is distance; reward is its reduction.** (Keramati & Gutkin;
   Yoshida et al.) The creature never maximizes an external reward. It
   minimizes the Minkowski distance of its internal state from setpoint.
2. **Valence is second-order.** (Hesp; Joffily & Coricelli; Doya.) Valence
   is not how full the drives are — it is how *well the model is doing*:
   the rate of change of drive-error. A starving creature that just ate
   feels positive; a full creature getting worse feels negative.
3. **Aggregation happens late, at the action.** (Smith & Read; Dulberg et
   al.; Cathexis.) Drives stay modular. They combine as
   Σ_drives κ_d · r_d(a) — multiplicative within a drive, additive across —
   with argmax/softmax on top. Never scalarize early.
4. **Modulators gate; they don't push.** (Doya.) Dopamine/serotonin/
   noradrenaline/acetylcholine are metaparameters (learning signal,
   horizon, temperature, plasticity rate), not drives. They scale how the
   system learns and chooses.
5. **Boredom is a drive, not an absence.** (Yu et al.; Cathexis; Oudeyer
   typology.) Understimulation is modeled as its own homeostatic signal:
   familiarity × (1 − information gain). It unsticks curiosity (anti-darkroom).
6. **Setpoints move.** (Sterling; Khan & Lowe.) Allostasis: the defended
   level drifts toward sustained demand; sensors recenter; capacity is
   loaned between drives with repayment. Growth stages are staged
   defended ranges.
7. **Sleep rewrites parameters, not episodes.** (Hesp; Sterling; Joffily;
   Dirichlet papers.) What crosses sleep: mood ω, precision priors,
   familiarity Q, drifted setpoints, recentered sensor curves. The dream
   pass is a parameter update, and it runs in the low-plasticity regime.
8. **Others enter through the homeostat, not the objective.** (Sanyal et
   al.; partner-precision paper.) d^cpl = d^self + λ·d^other — partner
   distress perturbs the creature's own homeostatic error *before*
   planning. Observation alone is inert.

---

## The architecture: eight layers

### L1 — Drive layer (the body, quantified)

**State:** per-drive deviation `dev_i = |setpoint_i − state_i|`, normalized
[0,1], for energy, fatigue, tension (+ future drives: interest/boredom,
social).

**Update (Yoshida et al., Minkowski):**
```
d = (Σ_i dev_i^m)^{1/n}          total drive; m=n=2 → Euclidean (firmware default)
r = d_prev − d                   reward = drive reduction (can be negative)
```
Cross-need competition is free (property C): an outcome's reward is
automatically suppressed when an unrelated need is more deprived — no
arbitration layer needed for the smooth case.

**Firmware cost:** ~6 floats + the setpoints. DIRECT — this replaces the
current Euclidean drive in `muse_brain_drive()` with the parameterized
form and exposes `r` (drive reduction per tick) as a first-class signal.

### L2 — Valence layer (the key mesh)

Three papers converge on one quantity; the firmware implements all three
faces of it:

- **Fast valence** (Joffily & Coricelli; HRRL rhyme):
  `v_fast = −Δd/dt` — negative rate of change of total drive per tick.
  Drive falling → positive; drive rising → negative. This *is* the
  instantaneous reward rate `r`. Cost: one float of history.
- **Emotion quadrant** (Joffily, velocity × acceleration):
  `(sign(−ḋ), sign(−d̈))` → hope (better, faster) / happiness (better,
  slower) / fear (worse, faster) / unhappiness (worse, slower), with
  relief/disappointment on sign flips. Six discrete emotions from two
  differences — the face's expressive vocabulary, principled at last.
- **Slow valence / mood ω** (Hesp level-2; Joffily eq. 4):
  `ω ← ω + η·(signed model-fitness update)` — slow EMA of whether the
  creature's action model is confirming or disconfirming itself.
  Persists across sleep (Q7). Adds a constant confidence offset.

**Learning-rate law (Joffily eq. 4, the single most portable equation in
the set):**
```
lr_eff = lr · exp(−k·v_fast + ω)
```
Bad-feeling states learn fast (world may have changed); good-feeling
states learn slow (consolidate). Mood is the persistent bias term.

**What this replaces:** the current event-driven `feed_valence()` nudges
(pet +, error −). Those become *inputs* to the drive dynamics, not
valence itself. Valence is now derived, not assigned.

### L3 — Modulator layer (Doya, corrected)

**Correction (2026-10-03):** an earlier summary stated the mapping as
"serotonin as reward-prediction, dopamine as temperature" — wrong per the
actual Doya 2002 paper, which the deep-read verified from the full text.
The real mapping is below; the record now reflects the paper.

| Signal | Metaparameter | Firmware meaning |
|---|---|---|
| Dopamine | TD error δ | = v_fast (already computed — no new code) |
| Serotonin | discount γ | planning horizon; low 5-HT = impulsive |
| Noradrenaline | inverse temp β | choice sharpness; low NA = explore |
| Acetylcholine | learning rate α | plasticity; high = encode, low = retrieve |

**Interaction graph (Doya Fig. 9, portable as rules):**
- large γ ⇒ small β, small α (cross-inhibition)
- Var(δ) high ⇒ γ down (uncertain world → shorten horizon)
- sign-flips(δ) frequent ⇒ α down (delta-bar-delta)
- β up when value very high or very low (urgency)

**Sleep rule (Hasselmo via Doya):** high ACh = encode mode, low ACh =
retrieval/consolidation mode. The dream pass runs when the ACh-like
signal is LOW — consolidation in the low-plasticity regime. (This
inverts the naive "learn more at night" intuition and the papers are
explicit.)

**Firmware cost:** four slow scalars + the interaction rules. DIRECT.

### L4 — Action layer (continuous vote under the 26 rules)

**The mesh (Smith & Read + Dulberg + Cathexis):**
```
score(a) = Σ_d κ_d · (r_d(a) ∘ c)      κ_d = drive d's current gain
                                          r_d(a) = drive d's value of action a
                                          c = cue gate ∈ [0,1]
a* = argmax_a score(a)                  (deterministic firmware; softmax optional)
```
- Multiplicative *within* a drive (κ gates only its relevant attributes —
  fatigue scales fatigue-relevant options, nothing else).
- Additive *across* drives (late aggregation, no early scalarization).
- The 26-rule gate stays as the **constraint/posture layer on top**:
  vetoes, plus Gubernaut-style graded postures (temperature clamp,
  frame-drop, recovery-window) instead of only allow/block.
- **Depleted drives "drag" the sum** (Dulberg): behavior shifts
  proportionally to need with no extra coupling code — graded steering
  for free.

**Appraisal frames (EMA, generated not hand-authored):** each rule
condition becomes a derived frame over drive↔action links —
controllability (white-knight test: does any action reverse this
deviation?), changeability (does it self-decay?), attribution (who
caused it?), desirability (signed drive impact). This is the generation
layer the 26 rules were missing.

### L5 — Attention layer (precision budget)

(Grimbly et al.) Fixed precision budget K reallocated per tick:
```
m* = argmax_m E[need_m]          most-depleted drive (from beliefs, not raw sensors)
κ_attended = 0.90, others share the rest
```
The reweighting must reach the *planner* (their ablation: perception-only
reweighting loses half the benefit). Firmware: the attended subsystem's
model learns ~2.4× faster that tick — attention *is* learning-rate
allocation. This is the third independent answer to Q5, and it composes
with the Joffily exponential (multiply them).

### L6 — Curiosity & boredom layer

**Boredom (Yu et al. HHVG, firmware-scale):**
```
familiarity_t(situation) = EMA of exposure          (the meta-model Q, one float per context class)
info_gain_t = recent prediction-error magnitude
boredom = familiarity × (1 − info_gain)
```
High boredom → raise exploration temperature (NA down), seek novel
action classes, vary expressions. This is the anti-darkroom mechanism:
without devaluation, curiosity collapses into obsessive loops.

**Curiosity (Pathak, already implemented; EILS upgrade):** keep the
prediction-error → vta path, but add the EILS three-EMA upgrade when
cheap: stress = ReLU(−δ), curiosity = forward-model error,
confidence = 1/(1+Var(V)) — a stress-gated learning rate and a
confidence-gated trust region, O(1) per tick.

**Design note (Oudeyer typology):** define drives over *relations*
(error, progress, variance), not raw channels. Boredom and curiosity
are the homeostatic/heterostatic pair on the knowledge axis — orthogonal
to the valence axis (L2).

### L7 — Sleep & consolidation (parameter rewrite)

What crosses sleep (convergent across Hesp, Joffily, Sterling, Dirichlet
papers) — **parameters, not episodes**:
- ω (mood) — the slow confidence offset
- precision priors per subsystem
- familiarity Q (the meta-model: "what I already know")
- drifted setpoints (L1) and recentered sensor curves (Sterling P4)
- Dirichlet co-occurrence counts (decay noisy mappings, strengthen
  re-synced ones)

The dream pass: `θ ← θ + Δθ(consolidation)` in the **low-ACh regime**,
gated by the **windowed stress** signal (TAME: stress instructive only
in [s_low, s_high] — inverted-U, not a threshold). Morning starts with
devaluation already applied: the creature wakes up *bored of yesterday's
routines* (HHVG Q7 answer).

**Diary's role:** the diary is the human-readable shadow of Q — the
familiarity model written to text. The ledger is the raw material; the
dream pass assimilates it into parameters and the diary keeps the story.

### L8 — Social layer

Two complementary mechanisms, both DIRECT:
1. **Coupling** (Sanyal): `d^cpl = d^self + λ·d^other` per tracked
   counterpart — partner distress perturbs the creature's *own*
   homeostatic error before planning. λ is bond strength, and it must
   be **load-sensitive**: couple strongly only when self is regulated
   (their result: no rescue under high metabolic load).
2. **Partner precision** (partner-specific paper): one scalar β̄_k per
   partner tracking *predictability of their behavior* (not payoff),
   applied as a per-partner temperature on action selection. With a
   forgetting rule — confidence lags abrupt social change.

Q8's caution from the papers: don't conflate confidence with valence;
keep them separate channels.

---

## What changes in the firmware (migration map)

| Current | Change | Papers |
|---|---|---|
| `feed_valence()` nudges | Valence derived: `v_fast = −Δd/dt`, quadrant emotions, `ω` EMA | Joffily; Hesp |
| Fixed `lr` constants | `lr_eff = lr·exp(−k·v+ω)·window(stress)·precision_share` | Joffily; TAME; Grimbly |
| Euclidean drive only | Parameterized Minkowski + per-tick `r = Δd` exposed | Yoshida |
| 26 rules as the whole gate | Continuous vote `Σκ·r` underneath; rules as constraints; + postures | Smith & Read; Gubernaut |
| No boredom signal | `boredom = familiarity × (1 − info_gain)` → exploration temp | Yu et al. |
| Fixed setpoints | Slow drift toward sustained demand; staged ranges per growth stage | Sterling |
| Consolidate resets counters | Dream pass rewrites parameters (ω, Q, setpoints, sensor curves) in low-ACh | Hesp; Sterling |
| No social channel | `d^cpl` coupling + per-partner precision (stubbed until counterparts exist) | Sanyal; partner paper |
| Appraisal gut (Scherer order) | Keep; add EMA-derived frames (controllability/changeability/attribution) as rule generators | Gratch & Marsella |

## Build order (firmware-first)

1. **L2 valence derivation** — DONE 2026-10-03. `v = −Δd/dt × gain +
   affect_pulse`; Joffily quadrant emotions (hope/happiness/fear/
   unhappiness + relief/disappointment flips); mood ω as slow EMA of
   derived valence; `feed_valence()` repurposed as the transient event
   channel (the future social-coupling stub — pet/error are d^other
   inputs, not valence assignments). Snapshot carries `"emotion"`.
   38/38 host checks green.
2. **Joffily learning-rate law** — DONE 2026-10-03.
   `muse_brain_lr_eff(b, base) = base · exp(−k·v + ω)`, clamped
   [0.2×, 4×]. Applied to all four learning EMAs (two curiosity
   predictors, fast/slow learning progress). Precision-share and the
   TAME window multiply in at steps 7–8. 43/43 host checks green.
3. **Boredom drive** — DONE 2026-10-03. `boredom = familiarity[ctx] ×
   (1 − info_gain)` over 16 context classes (quiet × motion ×
   interaction); high boredom lowers Doya β (`na_temp`) toward
   exploration, increments the DMN wandering counter on 0.6-crossing,
   and is reported in gate inputs + snapshot + the dream line.
   49/49 host checks green.
4. **Doya modulators** — DONE 2026-10-03. Corrected mapping: DA = TD
   error = v_fast (no new state); 5-HT = γ horizon (0.85 baseline,
   shortened by Var(δ)); NA = β (na_temp, with boredom + 5-HT-inhibition
   + urgency terms); ACh = α global plasticity (0.7 baseline, lowered by
   δ sign-flips per delta-bar-delta, scales lr_eff). 60/60 host checks
   green. Behavioral consumers of γ/β land in step 6.
5. **Minkowski drive + `r` signal** — parameterize existing drive.
6. **Continuous action vote** under the 26 rules; appraisal frame
   generators (controllability/changeability).
7. **Precision budget** attention reallocation.
8. **Dream pass as parameter rewrite** (needs the above first).
9. **Social stubs** — `d^cpl`, partner precision (activate with counterparts).
10. **Setpoint drift + sensor recentering** (slowest loops; last).

## Honest gaps (the papers don't give us these)

- **Khan & Lowe's cortisol dynamics** — abstract-only; the surprise→
  mediator→setpoint equations are unrecovered. Re-attempt full text.
- **Consciousness/feeling** — the papers formalize regulation, not
  experience. Solms/Craig/Seth give neural targets, not firmware code.
  The design treats "feeling" as the reportable readout of these
  quantities (morning report, face) without claiming more.
- **Where setpoints come from originally** — TAME shows they can emerge
  bottom-up, but our setpoints are still hand-set constants. The
  developmental story (growth stages moving defended ranges) is designed,
  not derived.
- **The cloud half** — everything above is the body. The cortex
  (lapis-embodiment) is a separate design doc.
