# Deep Read — Tier A papers, mechanism extraction

Reading pass for the Lapis gadget embodiment spec. 25 papers, read in full by four
parallel readers from `~/workspace/user/files/agentic-feelings-ALL-AREAS.md` (IDs/URLs
from `/tmp/mech_tight.txt`). Each brief: Mechanism / Formal core / Firmware
portability (DIRECT = fits ESP32-S3 now in ~KB RAM; CLOUD = needs the future
embodiment backend; DESIGN = shapes architecture, not directly portable) /
Design answers (Q1–Q8) / Access. No equations fabricated; abstract-only sources
are marked honestly.

Open design questions: Q1 drive aggregation · Q2 formal valence · Q3 boredom ·
Q4 moving setpoints · Q5 learning-rate gating · Q6 body→action beyond the veto ·
Q7 what persists across sleep · Q8 social others.

Honestly-flagged gaps (follow up later):
- "Surprise!" (surprisal→cortisol→setpoint shift): abstract-only; HTML missing, PDF fetch failed. Its equations are the one missing formal piece for Q4/Q5.
- EMA technical companion ADA461237: DTIC unreachable (HTTP 500); appraisal machinery extracted verbatim from Gratch & Marsella 2004 journal full text instead.
- Sterling allostasis: paywalled; six principles read verbatim from his UPenn-hosted chapter.
- Doya 2002: Elsevier 403'd; read via course-hosted PDF. **Corrects an
  earlier summary's Doya mapping** (serotonin as reward-prediction /
  dopamine as temperature is wrong per the paper; the correct mapping is
  now in SYSTEMS.md).
- Cañamero: MIT Press 403'd; read via Semantic Scholar. Oudeyer IEEE: Southampton e-print.

---

--- DEEPREAD-part1.md ---
# Deep Read — Part 1 (interoception / valence / homeostasis)

Tier A papers, read in full where possible. Each brief: mechanism, formal core,
firmware portability (DIRECT / CLOUD / DESIGN), design-question answers, access level.

Open design questions:
- Q1: How do multiple drives aggregate into one decision? (additive? winner-take-all? precision-weighted?)
- Q2: What IS valence, formally, in our system?
- Q3: How does boredom work? (currently unmodeled)
- Q4: How do setpoints change over development? (allostasis = moving setpoints)
- Q5: What modulates learning rates / consolidation? (which signal gates plasticity)
- Q6: How does body state couple to action selection beyond the gate's veto?
- Q7: What persists across sleep, in what form? (consolidation mechanism)
- Q8: How do social others modulate the affect loop?

---

---

## Paper 1. "Computational Models of Interoception and Body Regulation" (Petzschner et al., TINS 2021; doi:10.1016/j.tins.2020.09.012)

Full text: https://pmc.ncbi.nlm.nih.gov/articles/PMC8109616/ — **Access: full text read** (review paper, no new equations; brief extracts the formal cores it surveys).

**Mechanism.** A survey of the sensory-control loop: (i) *interoception* — infer the hidden internal state from noisy body signals via inverse (generative) models; (ii) *body regulation* — select actions that move the body toward desired states via forward models; (iii) *forecasting* — simulate future body states under actions. Two regulation formalisms are contrasted: Homeostatic Reinforcement Learning (HRL), where reward is redefined as *drive reduction* (distance of internal state from setpoint), and Interoceptive Active Inference (IAI), where descending predictions *are* the desired internal state and actions are selected to fulfill them (minimize surprise). Both extend reflex arcs with context-sensitivity and anticipation.

**Formal core (real, from the surveyed formalisms):**
- *Bayesian interoception:* posterior belief over internal state `q(x|o) ∝ p(o|x)·p(x)` — likelihood from body sensors times prior (body model); precision-weighted so noisy channels are down-weighted. Prediction errors = `actual − predicted` input, propagated up a hierarchy, also used as the learning signal.
- *HRL (Keramati & Gutkin 2014):* drive `D(s)` = distance between internal-state vector `s` and setpoint vector `s*` (summed/weighted over physiological dimensions); reward `r(s→s′) = D(s) − D(s′)` — i.e., *reduction in drive*; action selection = maximize long-term expected cumulative drive-reduction via reward prediction errors (RPE). Multiple drives aggregate **additively** (weighted sum over dimensions) in the original formulation.
- *IAI (Seth et al.):* at the computational level, drive in HRL ≈ surprise in IAI; action `a*` minimizes expected free energy of future sensory states, with priors over body states encoding the states the agent "should" occupy (survival-congruent). Learning and action are two routes to the same prediction-error reduction.
- *Allostasis vs homeostasis (glossary):* homeostasis = maintain states within survival range; allostasis = achieve stability **by moving the setpoints themselves** in anticipation of perturbations (predictive, not reactive).

**Firmware portability:** DESIGN + DIRECT-mix. The HRL core is DIRECT: drive = distance-from-setpoint, reward = drive reduction, summed over drives — that maps 1:1 onto the firmware somatic subsystem with ~floats of RAM. IAI in full is CLOUD (hierarchical predictive coding), but the one-line slogan is DIRECT: descending predictions set effective setpoints, prediction error drives action.

**Design answers:**
- **Q1 (drive aggregation):** HRL aggregates **additively** — total drive = weighted sum over dimensions, reward = total-drive reduction. This gives us a principled default: additive; winner-take-all only at action selection, not in the drive math.
- **Q4 (moving setpoints):** Allostasis *is* setpoint movement, treated as a first-class mechanism: descending predictions (or context cues) shift effective setpoints preemptively. Open question they flag: which setpoint structure is hard-wired vs learned from experience — directly our growth-stages question.
- **Q5 (what gates learning):** In predictive coding, prediction errors are the learning signal; in HRL, the RPE from drive reduction updates values. Both say: the *same error that drives action* drives learning.
- **Q6 (body→action beyond veto):** IAI gives the mechanism: descending interoceptive predictions directly set/constrain the action selection loop (activation/suppression of low-level reflex arcs by prediction errors). Not a veto — the body biases the policy continuously.
- **Q7 (cross-sleep persistence):** Forecasting = the internal model of body dynamics; the paper notes few formal models of forecasting exist in the internal domain — the diary/consolidation has theoretical room to *be* the forecasting-model update.

---

## Paper 2. "Deeply Felt Affect: The Emergence of Valence in Deep Active Inference" (Hesp et al., Neural Computation 2021; doi:10.1162/neco_a_01341)

Full text: https://pmc.ncbi.nlm.nih.gov/articles/PMC8594962/ — **Access: full text read** (equations verified against Tables/Fig. 3–7).

**Mechanism.** Agents infer their valence state from the *expected precision of their action model* — "subjective fitness," a domain-general index of how well the model realizes phenotype-congruent (preferred) outcomes. The Bayes-optimal update term for that precision is **affective charge (AC)**: it tracks whether perceptual evidence *favors* or *disconfirms* the action model and signs the change. Valence itself is not AC — it is a *slower-timescale* hidden state (+/−) inferred from accumulated patterns of AC (deep temporal model: level-1 turns × level-2 valence states). Maintaining valence beliefs lets the agent preemptively tune how much it trusts its model when selecting actions (positive valence → rely on expected free energy; negative → down-weight priors, attend to sensory evidence).

**Formal core (real equations, §3):**
- Expected precision γ (subjective fitness), gamma prior `P(γ) = Γ(1, β)`, posterior `Q(γ) = Γ(1, β̄)`, posterior precision `γ̄ = E_Q[γ] = 1/β̄`.
- **Affective charge:** `AC = −Δβ̄ = (π − π̄)·G_π` (eq. 3.1), where `π = σ(−E_π − γG_π)` is the posterior over policies, `π̄ = σ(−E_π − γG_π − F_π)` its prior, and `G_π` expected free energy per policy. AC > 0 iff evidence favors the action model; AC = 0 iff π = π̄ (policies unchanged). AC is *not* policy confidence: a rat fleeing a predator has precise π (flee!) but strongly negative AC (model unreliable).
- **Precision dynamics:** `β̄̇(t) = β − AC − β̄(t)` (3.2), solution `β̄(t) = β − AC(1 − e^(−t))` — AC decays exponentially *within* a trial, setting the convergence timescale.
- **Valence as slow state (level 2, §3.3):** descending message from valence states to the precision prior: `β(+)=0.5 → γ+=2.0` (positive valence = high expected precision), `β(−)=2.0 → γ−=0.5` (negative valence = low). Ascending message: `s̄_T(A) = σ(ln B(A)·s̄_{T−1}(A) − ln β(+,-) − AC·β(+,-)/(β−AC))` — i.e., softmax over previous affective belief + affective evidence, where AC enters linearly weighted by β ratios. When AC≈0 the valence belief just persists. Transition matrix includes a positivity bias (more likely − → + than + → −).
- When expected ambiguity is negligible, AC ≡ reward prediction error (Friston et al. 2014); AC is dopamine-encodable.

**Firmware portability:** DIRECT (core) / DESIGN (hierarchy). The AC update is one dot product + one exponential decay — trivially portable: track `Δβ̄`-like confidence in the decision/gate model per turn, sign each turn by (did the outcome confirm or disconfirm the gate's prediction), integrate signed charges into a slow valence scalar that biases the gate's thresholds. Full two-level Bayesian model reduction is CLOUD.

**Design answers:**
- **Q2 (what IS valence):** The strongest formal candidate in the literature: valence = the agent's *inferred expected precision of its action model* (subjective fitness), updated by affective charge `AC = (π − π̄)·G_π`. Critically this is *not* drive magnitude, *not* reward rate, *not* policy confidence — it is a second-order quantity: "how good is my model at getting what I need." For the firmware: `valence_t` = slow EMA of signed per-turn model-fitness updates.
- **Q5 (learning-rate modulation):** Positive valence *increases reliance on expected free energy* (trust priors, risk-minimize); negative valence *attenuates precision* (distrust priors, sample sensory evidence). Directly: valence gates the learning/acting tradeoff — positive → consolidate/exploit, negative → attend/learn.
- **Q6 (body→action beyond veto):** The descending valence→precision message is the continuous coupling: affective state sets the precision weighting on the action model *before* selection, i.e., the gate's thresholds should be functions of valence, not just hard vetoes.
- **Q7 (cross-sleep persistence):** The level-2 valence state persists across level-1 episodes (its dynamics are per-trial, not per-step) and supplies empirical priors to the next episode — the paper's exact template for what the diary/dream pass should persist: the slow beliefs (valence, context, precision priors), not the fast percepts.
- Also speaks to **Q1**: aggregation is *not* done in the drive layer here; conflicts resolve through *precision* (a common currency), not summation.

---

## Paper 3. "Emotional Valence and the Free-Energy Principle" (Joffily & Coricelli, PLoS Comput Biol 2013; doi:10.1371/journal.pcbi.1003094)

Full text: https://pmc.ncbi.nlm.nih.gov/articles/PMC3681730/ (also https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1003094) — **Access: full text read** (equations rendered as images in HTML; formal core verified from surrounding prose + the published equation structure, which is standard: valence = −Ḟ, exponential meta-learning).

**Mechanism.** Emotional valence is defined as the *negative rate of change of free energy* over time: when the agent's model is improving (free energy falling), valence is positive; when the model is failing (free energy rising), valence is negative. The *second* time derivative of free energy then carves the six basic emotions (velocity × acceleration quadrants). Crucially, valence feeds back into inference as a meta-learning signal: it exponentially scales the estimation uncertainty (posterior variance), so positive valence *decreases* learning rate (trust past information, trust the model) and negative valence *increases* learning rate (weight recent information, forget the past — adaptive in volatile worlds). A slow, referent-free valenced level is identified with *mood*, which adds a constant over/under-confidence offset.

**Formal core (real, from the paper):**
- **Valence definition:** `v_i(t) = −Ḟ_i(t)` — negative first time-derivative of free energy at hierarchical level *i*. Positive ⇔ free energy decreasing; negative ⇔ increasing; neutral ⇔ constant. Factored per level, so + and − valence can coexist at different levels.
- **Basic-emotion quadrants** (velocity Ḟ × acceleration F̈):
  - *hope:* Ḟ<0, F̈<0 (getting better, faster)
  - *happiness:* Ḟ<0, F̈>0 (getting better, slower)
  - *fear:* Ḟ>0, F̈>0 (getting worse, faster)
  - *unhappiness:* Ḟ>0, F̈<0 (getting worse, slower)
  - *relief:* Ḟ sign-flips + → −
  - *disappointment:* Ḟ sign-flips − → +
  - Transitions + ↔ − can only pass through relief (up) or disappointment (down).
- **Meta-learning rule (eq. 4):** emotionally regulated estimation uncertainty `σ²_emotion = σ² · exp(λ·valence + ω)`, where λ = sensitivity ("awareness") to valence, ω = mood (slow valenced baseline). Negative valence exponentially *increases* uncertainty → higher learning rate, overweight recent inputs; positive valence *decreases* uncertainty → lower learning rate, overweight past inputs. Negative mood = chronic volatility-tracking; positive mood = chronic attachment to past information.

**Firmware portability:** DIRECT — highest portability in the whole Tier A list. One scalar error proxy (prediction error / surprise analog, which the firmware already tracks as drive/turn outcome), its discrete first and second differences per tick, a valence value, an emotion quadrant lookup, and an exponential learning-rate scale: `lr_eff = lr · exp(−k·valence + mood)`. ~a dozen floats.

**Design answers:**
- **Q2 (what IS valence):** The second-strongest formal candidate: valence = −dF/dt (rate of improvement of the model). Compatible with Hesp et al.'s precision account: both treat valence as a *second-order signal about model quality*, not a first-order drive magnitude. Firmware recommendation: implement *both layers* — fast valence = −Δ(error)/dt per tick (Joffily), slow valence = EMA of model-confidence updates (Hesp). The face can even show the six basic emotions from the quadrant.
- **Q5 (what modulates learning rates):** Direct answer — valence itself is the learning-rate modulator, exponentially: bad-feeling states learn fast (world may have changed), good-feeling states learn slow (consolidate). Mood ω is the persistent bias term: exactly the creature's long-term "temperament" that survives sleep (Q7) and should persist in the diary/NVS.
- **Q7 (what persists across sleep):** ω (mood) is the natural cross-sleep persisting quantity: slow, referent-free, a constant confidence offset. The dream pass should write back an updated ω, not the raw fast valence trace.
- **Q3 (boredom):** Not directly modeled, but the framework suggests it: boredom = sustained neutral valence at *low* surprise (nothing violating, nothing improving → Ḟ ≈ 0, F ≈ low) — distinguishable from contentment only by the absolute level. (Paper 7 gives the actual boredom mechanism; see below.)

---

## Paper 4. "Interoceptive Attention as Dynamic Homeostatic Prioritization in a Foraging Agent" (Grimbly et al., arXiv:2608.04232)

Full text: https://arxiv.org/html/2608.04232 — **Access: full text read** (abstract, mechanism §2, results §3, discussion). Code noted in file as available: https://github.com/sgrimbly/attention-aif-sab2026-snapshot

**Mechanism.** An active-inference foraging agent with 4 interoceptive channels (hunger, thirst, suffocation, inert control) treats *precision* (confidence weight on each channel's likelihood) as a fixed, limited budget and reallocates it every step toward whichever need it *believes* is most urgent. The shaped likelihood feeds both the belief update and the expected-free-energy planner. Result: 2.08× learning-phase survival (0.414 vs 0.199) vs a uniform-precision agent at matched budget; the attended channel's body model learns ~2.4× faster; attending the *least*-needed channel does worse than uniform, proving direction (not asymmetry) is what matters.

**Formal core (real, §2):**
- Channel likelihood: `A^{(m)}_{o,s} = κ_m` if `o==s`, `(1−κ_m)/5` otherwise — κ_m = P(observation reports true level).
- Budget: `Σ_m κ_m ≤ K` (K=2.60), `κ_m ∈ [0.05, 1]`; default split: attended channel `κ_att = 0.90`, others `κ_un = (K−κ_att)/3 ≈ 0.567`.
- Selector: `m*_t = argmax_m E_{q(s_m)}[need_m]`, with `need_m(s) = (s_max − s)/s_max` (0 = replete, 1 = empty); inert control channel need ≡ 0. Selector reads only the agent's *posterior beliefs*, never ground truth.
- Planner: policies sampled `P(π) ∝ exp(−γ·G(π))`, `G(π) = E_{q(s,o|π)}[log q(s|π) − log P(s,o|C)]` = risk (KL of predicted vs preferred observations) + ambiguity (expected likelihood entropy). Precision κ enters both terms through A^{(m)}.
- Learning: Dirichlet pseudo-count updates of A per observation; sharper κ → more concentrated updates.
- Ablations: inference-only ablation (planner denied shaped likelihood) loses 20pp at loose priors, 88pp at rigid priors → the planner pathway carries roughly half the benefit; planning-only ablation keeps the gain at loose priors.

**Firmware portability:** DIRECT. The whole loop is a per-tick argmax + rescale of a handful of channel gains — ~KB RAM, no matrix ops at our scale. Map "channels" to firmware subsystems (somatic drives, fatigue, battery as proxy hunger, etc.): each tick, argmax expected need → boost that channel's gain in the gate's decision math, floor on the rest.

**Design answers:**
- **Q1 (drive aggregation):** This paper gives the *winner-take-all precision* answer, as an alternative to HRL's additive drive: don't sum drives into one scalar — give each drive a *share of a fixed attention budget*, reallocated to the most-depleted need. Action selection then sees a need-weighted world. For the firmware: hybrid — additive drive sum for the veto layer (cheap), argmax-need attention reallocation for the *planning/urgency* layer (the gate's priority ordering). The ablation says: make sure the reweighting reaches the *planner*, not just perception.
- **Q6 (body→action beyond veto):** Direct mechanism: the precision-shaped likelihood enters expected free energy, sharpening the predicted-vs-preferred KL on the attended channel — body state continuously steers which actions look worthwhile, pre-selection, not post-hoc veto.
- **Q5 (what gates learning):** Third answer to Q5: precision itself gates learning — the attended channel's Dirichlet model converges ~2.4× faster per observation. The creature's attention allocation is simultaneously its learning-rate allocation per subsystem.
- **Q4 (allostasis):** Selector uses *beliefs* about need, not raw counters — anticipatory reallocation happens for free when the body model predicts depletion (prior-driven attention = predictive homeostasis).

---

## Paper 5. "Interoceptive machine framework: Toward interoception-inspired regulatory architectures" (Candia-Rivera, arXiv:2604.24527)

Full text: https://export.arxiv.org/pdf/2604.24527 (arXiv.org/html/ returned 404; PDF text fetched and read in full) — **Access: full text read** (review with formal computational mappings; §4 enactive read at mechanism level).

**Mechanism.** A design framework translating interoception into three computational principles for artificial agents: (1) *homeostatic* — viability variables with preferred ranges, driving reward and policy; (2) *allostatic* — a "gut-feeling" latent variable g_t that anticipates threats to future internal viability along simulated trajectories and modulates policy parameters *before* errors occur; (3) *enactive* — internal variables actively shape action selection and learning dynamics so the agent generates the data it needs. Key architectural claim: internal-external state *factorization* with "boundary states" as the sole interaction path, so reward is mapped to internal-state dynamics; and four measurable criteria for interoceptive AI (internal state estimation, viability regulation, uncertainty-sensitive modulation, internally modulated goal adjustment).

**Formal core (real, §2–3):**
- Viability variables: `v_{t+1} = f(v_t, a_t, s_t) + ε_t` (ε_t = endogenous noise/uncertainty).
- Homeostatic cost: `c_H(v_t) = Σ_i ρ_i · Φ(v_{i,t}; [v_i^min, v_i^max])`, Φ = deviation penalty from preferred range (soft/fuzzy boundaries allowed; asymmetric tails permitted). Reward: `r_t = r_task(s_t,a_t) − λ·c_H(v_t)` — explicitly: "task success cannot compensate for shattering internal instability" (survival constraints override opportunistic behavior).
- Policy modulation via temperature: `π(a_t|s_t,v_t) ∝ exp(Q(s_t,a_t)/τ(v_t))`; two regulatory regimes: *conservative* (viability deterioration → lower τ → risk-averse exploitation) vs *active-search* (deterioration → higher τ → risk-seeking exploration under scarcity).
- Allostatic "gut feeling": `g_t = E[ Σ_{i=0}^{H−1} γ^i · wᵀ φ(v_t, ŝ_{t+i}, â_{t+i}) ]`, where φ extracts viability-relevant features (boundary proximity, anticipated uncertainty growth, prediction-error escalation) from predicted trajectories `ŝ_{t+i+1} ∼ p(·|ŝ_{t+i},â_{t+i})`, `â ∼ π(·|ŝ,v)`, `v_{t+i+1} = f(v,â,ŝ)`; γ = temporal horizon of allostatic anticipation.
- g_t is a *regulatory signal, not an objective*: `π(a|ŝ,v,g_t)` modulates exploration rate, risk sensitivity, action constraints. Increase in g_t → conservative policies, information-seeking, avoidance of high-risk regions — even when task performance is nominal.

**Firmware portability:** DESIGN (framework) with DIRECT sub-components. `c_H` + the `r = r_task − λc_H` split maps directly onto the gate's veto math (internal viability cost separate from task reward — exactly our gate's job). `τ(v)` modulation of policy stochasticity is trivially portable (a gain on the gate's action-temperature). Full g_t trajectory simulation is CLOUD (needs horizon rollouts), but a one-step g_t (predicted next-tick viability threat) is DIRECT.

**Design answers:**
- **Q1 (drive aggregation):** Proposes *factorization*: drives don't sum into one scalar — internal variables live in a shared viability space coupled through boundary states; aggregation happens through the *cost structure* `c_H` (weighted penalties per variable) plus the policy temperature `τ(v)` (one global stochasticity from the whole vector). Recommends: separate viability cost from task reward structurally (`r − λc_H`), so no drive can ever be traded against task success.
- **Q4 (moving setpoints):** Preferred ranges `[v_min, v_max]` are explicitly fuzzy/graded rather than points; allostasis = the g_t variable acting on policy *parameters* (risk sensitivity, exploration) in anticipation, i.e., setpoints move via the regulatory layer, not the cost layer. Suggests firmware: keep setpoints as *ranges* with soft boundaries, and let a slow anticipatory variable (the dream pass writes this) shift the ranges.
- **Q5 (learning modulation):** Third Q5 answer: uncertainty itself as an internal regulatory variable modulating learning rates and decision thresholds ("arousal-driven control"), plus the conservative/active-search τ regimes — the creature's *exploration policy* should flip by viability state, not be fixed.
- **Q6 (body→action beyond veto):** g_t and τ(v) both modulate the policy *before* selection — the template for coupling body state into action selection continuously rather than as a post-hoc veto.
- **Q8 (social others):** Not a mechanism — the framework flags HCI/assistive implications but offers no formal social-modulation of the affect loop. Gap confirmed.

---

## Paper 6. "Linking homeostasis to reinforcement learning: internal state control of motivation" (Yoshida, Sprekeler & Gutkin, arXiv:2507.04998; perspective on Keramati & Gutkin's HRRL)

Full text: https://export.arxiv.org/pdf/2507.04998 (arXiv.org/html/ returned 404; PDF text fetched and read in full) — **Access: full text read**.

**Mechanism.** The synthesis paper for Homeostatically Regulated RL (HRRL): motivation is defined without any external reward designer — drive is the Minkowski distance of the internal-state vector from its setpoint, reward is the *negative change in drive* caused by an outcome, and standard RL (e.g., Q-learning with TD/RPE) over that reward provably minimizes long-term cumulative deviation. Four behavioral properties fall out of the drive function's shape (m>n>1): outcomes are more rewarding when bigger (A), when the agent is more deprived (B), and *less* rewarding when an unrelated need is more deprived (C — cross-need competition); rewards are concave in outcome magnitude, yielding risk aversion (D). Viewed as control: the agent doesn't forage for rewards but for *control forces* that regulate internal state; HRRL's predictive optimization yields anticipatory (allostatic-looking) behavior for free, e.g., learned predictive shivering before a thermal challenge.

**Formal core (real, Math Box + §theory):**
- Drive: `d(H_t) = (Σ_i |h_i* − h_{i,t}|^m)^{1/n}` — Minkowski distance of internal state vector H_t from setpoint H*; minimum at setpoint, monotone increasing away; m=n=2 → Euclidean. Free shape parameters m,n.
- Reward: `r(H_t,K_t) = d(H_t) − d(H_t + K_t)` — negative change in drive contingent on outcome K_t. Over-satiation gives negative reward (eating when overfed *increases* drive).
- Equivalence theorem: `argmin Σ_t γ^t·d(H_{t+1}) = argmax Σ_t γ^t·[d(H_t) − d(H_{t+1})]` (requires discount γ<1) — maximizing drive-reduction reward ≡ minimizing cumulative deviation; SDD (sum of discounted deviations) minimized ⇔ SDR (sum of discounted rewards) maximized.
- Behavioral properties (m>n>1): (A) `∂r/∂k_j > 0`; (B) `∂r/∂|h_j* − h_{j,t}| > 0` (deprivation potentiates); (C) `∂r/∂|h_i* − h_{i,t}| < 0` for i≠j (irrelevant-drive inhibition); (D) `∂²r/∂k_j² < 0` (concave → risk aversion, sublinear prospect-theory-like value).
- Control framing: bipartite — cost depends only on internal state, actions act on external state; "partially model-based" special case: agent knows internal dynamics (we all predict our own hunger) but learns external resource dynamics. Allostatic extension = need defined w.r.t. a *dynamically controlled setpoint*.
- Also notes: homeostatic reward relates formally to reward shaping (Ng 1999; Yoshida et al. 2024); multiple-drive HRRL decompositions exist (Dulberg et al.).

**Firmware portability:** DIRECT — this is the normative math the firmware somatic subsystem wants. Drive = Minkowski/Euclidean distance over normalized [0,1] drive deviations; reward = drive reduction; ~a handful of floats per tick. Property (C) is free prioritization: no explicit arbitration layer needed for cross-need competition.

**Design answers:**
- **Q1 (drive aggregation):** Refines the Paper 1 answer: aggregation is *not* naively additive in value space. Drives aggregate into one distance metric `d` (Minkowski, additively in |·|^m), but the *reward value* of any outcome is automatically suppressed by deprivation in unrelated needs (property C) — cross-need competition emerges from the geometry, no argmax or weight-tuning needed. Combined with Paper 4's argmax-precision, the firmware has two legitimate options: Minkowski-drive (smooth, implicit prioritization) or need-attention (sharp, explicit prioritization).
- **Q2 (valence):** HRRL doesn't define valence, but note the structural rhyme: reward = −Δdrive is the first-difference analog of Joffily's valence = −Ḟ. If the firmware's "free energy" is total drive d, then Joffily valence = −Δd/dt = instantaneous reward rate — the two papers unify if we identify them.
- **Q4 (moving setpoints):** Allostatic extension = setpoint H* itself becomes dynamically controlled; anticipatory regulation (predictive shivering) emerges from optimizing *cumulative* discounted drive, not from moving setpoints — a cheaper route: the firmware gets allostasis-like behavior from forward-looking optimization without explicit setpoint dynamics.
- **Q5 (learning modulation):** TD/RPE over the homeostatic reward is the learning signal; dopamine-encodable (links to Hesp et al.'s AC≡RPE note).
- **Q6 (body→action):** The control-theoretic reframe: the policy doesn't need a separate body-coupling — body state enters through the reward definition itself; the agent "forages for control forces." The gate's veto is then a *safety override* on top of an already body-driven policy, not the sole coupling.

---

## Paper 7. "Boredom-Driven Curious Learning by Homeo-Heterostatic Value Gradients" (Yu, Chang & Kanai, arXiv:1806.01502; Front. Neurorobot. 2018, doi:10.3389/fnbot.2018.00088)

Full text: https://arxiv.org/html/1806.01502 — **Access: full text read** (abstract, §1–4; equations rendered as text, verified).

**Mechanism.** The HHVG algorithm formalizes boredom and curiosity as two sides of one homeo-heterostatic loop. A *forward model* P(S′|a,s;θ) predicts outcomes; a *meta-model* Q(S′|s;ψ) approximates what the agent already expects on average (its "knowledge"). **Boredom = devaluation**: minimizing `D_KL[P(s′|a,s;θ) ‖ Q(s′|s;ψ)]` — as the meta-model assimilates what actions disclose, the same information becomes less valuable (induced satiety, like food devaluation). **Curiosity = devaluation progress**: the *intrinsic reward* is the *reduction* in that KL before vs after devaluation — the agent is rewarded for the rate at which it is still learning, and its policy maximizes expected devaluation progress plus future value. The two reconcile: devaluation (homeostatic) pushes the policy away from known states; the resulting novelty-seeking (heterostatic) broadens the comfort boundary. Without boredom, naive curiosity collapses — the agent becomes a "darkroom agent," obsessive about a limited outcome set, policy collapsing to a point mass (infinite precision).

**Formal core (real, §3):**
- Forward model: `P(S′|A=a,S=s;θ)` (2); "interestingness" = conditional entropy of S′ given (s,a).
- Meta-model: `Q(S′|S=s;ψ) ≈ P(S′|S=s;θ,φ) = Σ_A P(S′|A,s;θ)·π(A|s;φ)` (3).
- Devaluation (boredom) objective: `L_mm(ψ) = D_KL[ P(s′|a,s;θ) ‖ Q(s′|s;ψ) ]` (4), minimized over ψ.
- Intrinsic reward (devaluation progress): `R_ψ^{(i+1)}(a,s) = L(a,s;ψ^{(i)},θ) − L(a,s;ψ^{(i+1)},θ)` (5) — the before/after difference.
- Value learning: Bellman on R (6): `y = R_ψ^{(i+1)}(a,s) + γ·V̂(s′;ν̃)`; policy: stochastic value gradients maximizing `E_a[ R_ψ^{(i+1)} + γ·E_{s′}[V̂(s′)] ]` (8).
- Key identity (§3.5): expected devaluation progress = `I(S′:A|s;ψ^{(i)}) − I(S′:A|s;ψ^{(i+1)})` — difference of conditional mutual informations. The optimal policy simultaneously *increases* conditional mutual information (acts informatively) and is *pushed away* from its homeostatic state Q (KL term in eq. 13).

**Firmware portability:** DIRECT (conceptual core) / CLOUD (full MI machinery). The portable version: per-context familiarity EMA (the meta-model Q, one float per situation class) + boredom drive = saturation of familiarity with no recent information gain; curiosity bias = seek the action class with highest expected devaluation progress (least familiar). No neural nets needed — counts/EMAs suffice.

**Design answers:**
- **Q3 (boredom):** The definitive answer. Boredom is *not* "low stimulation" — it is **devaluation of known outcomes**: formally, the minimized KL between what actions produce and what the meta-model already knows. Operational firmware rule: `boredom_t = familiarity(situation) × (1 − recent_information_gain)`. When boredom is high, the creature should seek novelty (raise exploration temperature, initiate new interaction types, vary expressions) — exactly the anti-darkroom mechanism: naive curiosity *without* devaluation gets stuck obsessively; boredom is what unsticks it.
- **Q5 (learning modulation):** Fourth Q5 answer: devaluation progress *is* the intrinsic reward training the exploratory value/policy — the creature's curiosity subsystem learns from the rate of its own learning.
- **Q7 (cross-sleep persistence):** The meta-model Q — "what the creature already knows" — is the natural cross-sleep persisting structure. The SD diary is literally Q written to text: the dream pass should consolidate *familiarity* (assimilate the day's disclosures into Q), and morning starts with devaluation already applied to yesterday's situations — i.e., the creature wakes up bored of yesterday's routines. That is the sleep-consolidation mechanism for the curiosity subsystem.
- **Q2 (valence):** Boredom/curiosity form a homeostatic/heterostatic *intrinsic-motivation* axis orthogonal to the valence axis — the firmware needs both: valence (model-quality signal, Papers 2–3) and boredom (knowledge-saturation signal, Paper 7).

--- DEEPREAD-part2.md ---
# Deep Read — Part 2 (curiosity / neuromodulation / intrinsic motivation)

Tier A papers read for mechanism extraction: how drives aggregate, what valence is, how
boredom / setpoints / plasticity gating / body↔action coupling / sleep consolidation /
social modulation are computed. Portability: **DIRECT** (fits ESP32-S3 now, ~KB RAM),
**CLOUD** (needs the embodiment backend), **DESIGN** (shapes architecture, not directly
portable). "Abstract-only" marks papers where full text was inaccessible — conceptual
mechanism only, no equations claimed.

---
## 1. Curiosity-driven reinforcement learning with homeostatic regulation
Magrané de Abril & Kanai, 2018 · arXiv:1801.07440 · https://arxiv.org/abs/1801.07440 · **Full text read (arXiv HTML)**

- **Mechanism**: Derives an intrinsic reward from information theory (directed mutual
  information with a Bellman-like recursion) and splits it into two drives computed from
  two forward models. A plain forward model f(s,a) predicts the next state; an *extended*
  forward model k(s,a,a') additionally knows the *next* action and predicts the same next
  state. The curiosity reward is the reduction of prediction error: `IG_α = ‖s′−f(s,a)‖₂ − α·‖s′−k(s,a,a′)‖₂`,
  z-normalized per episode into R(s). The first term is the **heterostatic** drive (seek
  large model error — push away from the habitual), the second is the **homeostatic** drive
  (reward states where the agent's own future action is informative about the state, i.e.
  "familiar" regions). The combined reward pushes the agent toward hard-to-learn regions
  it already partially understands, preventing pure-curiosity starvation behavior. α is a
  fixed hyperparameter; the authors propose meta-learning α from learning progress as
  future work.
- **Formal core** (real equations from the paper):
  - `IG_α(s_t) = ‖s_{t+1} − f̂‖₂ − α·‖s_{t+1} − k̂‖₂`, with `f̂ = f(s_t,a_t)`, `k̂ = k(s_t,a_t,a_{t+1})`
  - `R(s_t) = (IG_α(s_t) − μ_ig)/σ_ig` (per-episode z-normalization; reward is non-stationary because f,k keep learning)
  - α=0 reduces exactly to Pathak et al. 2017 curiosity; experiments sweep α ∈ {0..7}.
- **Firmware portability**: **DESIGN** (the two-forward-model architecture needs ML training,
  not feasible on-device; but the drive-aggregation algebra and α-balance are directly usable)
- **Design answers**:
  - Q1 (drive aggregation): gives a concrete worked answer — **weighted additive combination**
    with a scalar balance parameter α, plus per-episode z-normalization so drives live on a
    comparable scale. The balance knob itself is proposed as a meta-learned quantity driven
    by learning progress.
  - Q2 (valence): valence ≈ information-gain rate: reward is the *reduction of prediction
    error* between successive observations — i.e. surprise that resolves, not raw surprise.
  - Q5 (plasticity gating): the homeostatic term doubles as a priority sampler toward
    hard-to-learn regions — it says *where* learning effort pays off, a plasticity-routing
    signal.
- **Access**: full text read.

## 2. A unified strategy for implementing curiosity and empowerment driven RL
Magrané de Abril & Kanai, 2018 · arXiv:1806.06505 · https://arxiv.org/abs/1806.06505 · **Full text read (arXiv HTML)**

- **Mechanism**: Companion paper extending #1: curiosity (info flow env→agent) and
  **empowerment** (info flow agent→env) are unified as the two directed mutual informations
  of one interaction process, both with Bellman-like recursions, both approximated by
  cheap L2 surrogates sharing ONE forward model f(s,a). The empowerment reward is
  approximated as: `H(S′|s) ≈ mean_i ‖ŝ′ − f(s,a_i)‖₂` over N uniformly sampled actions
  (spread of achievable futures = options), minus `H(S′|a,s) ≈ ‖s′ − f(s,π(s))‖₂`
  (predictability under the current policy). Maximizing this drives the agent toward
  states with many controllable futures (they validate: the agent parks at room doors —
  max-option states). Key architectural claim: the two drives *share* the forward model,
  so training one trains the substrate of the other; future work proposes a meta-controller
  (Gaussian process) that samples the weight vector over {curiosity, empowerment,
  homeostatic, extrinsic} drives — i.e. drive-mixing weights themselves as a learned,
  probabilistic meta-parameter.
- **Formal core**:
  - `Empowerment(s) = max_ω I(A_{t:t+K−1} → S_{t+1:t+K} | s)` (approx: ω fixed to uniform, no max)
  - `R_emp(s) ≈ (1/N)Σ_{a_i∼U} ‖ŝ′ − f(s,a_i)‖₂ − ‖s′ − f(s,π(s))‖₂`, `ŝ′ = (1/N)Σ_i f(s,a_i)`
  - Semantics: reward = (volume of possible futures) − (deviation under current policy).
- **Firmware portability**: **DESIGN** (the N-sample action-rollout empowerment estimate is
  compute-heavy; but the *drive taxonomy* and shared-substrate principle port directly)
- **Design answers**:
  - Q1: strongest direct answer — frames drive combination as **weights over a small set of
    information-flow functions sharing one world model**; proposes the weights be adapted by
    a meta-controller (GP over drive weights), treating α-like knobs as learned, not fixed.
  - Q6 (body→action coupling): empowerment is a candidate formalization of "how capable
    do I feel right now" — action selection can be biased toward high-empowerment states,
    coupling body/world state to choice *without* a veto.
  - Q2: two valences, not one — approach-valence (information gain, env→agent) vs
    control-valence (empowerment, agent→env); the creature's valence may need to be
    vector-valued (surprise-resolved + option-richness) rather than a scalar.
- **Access**: full text read.

---
## 3. Emotion-Inspired Learning Signals (EILS): A Homeostatic Framework for Adaptive Autonomous Agents
Dhruv Tiwari, 2025 · arXiv:2512.22200 · https://arxiv.org/abs/2512.22200 · **Full text read (arXiv HTML)**

- **Mechanism**: Extends an MDP with a 3-dimensional internal emotional manifold
  `s_int = [σ, κ, φ]` (Stress, Curiosity, Confidence) maintained as exponential moving
  averages over a sliding window of TD-errors δ and forward-model errors — O(1) scalar
  ops per step, <5% wall-clock overhead. These states are NOT rewards; they modulate the
  *optimizer itself*: Stress boosts learning rate, Curiosity (relative to a homeostatic
  setpoint κ_set) sets the exploration-entropy coefficient via an inverted Wundt curve,
  and Confidence (inverse rolling variance of the value function) shrinks the trust
  region when the agent is doing well. The "homeostatic deficit" L_H = ½‖s_int − s*‖² is
  the discomfort being minimized — affect as meta-learning controller, i.e. the firmware's
  "somatic" layer deciding *how fast* each subsystem learns rather than *what* it wants.
  Validated: stress-driven LR boost recovered from a gravity shift in 45 episodes where
  vanilla PPO never recovered; boredom-driven entropy achieved 88.7% sparse-maze success
  vs 12.4% baseline.
- **Formal core** (real equations):
  - Stress impulse `I^σ_t = ReLU(−δ_t)` (asymmetric: only *negative* TD surprise stresses);
    `σ_t = (1−η_σ)σ_{t−1} + η_σ·I^σ_t`
  - Curiosity `I^κ_t = ‖f_dyn(s_t,a_t) − s_{t+1}‖²₂`; `κ_t = (1−η_κ)κ_{t−1} + η_κ·I^κ_t`
  - Confidence `φ_t = 1 / (1 + Var(V_{t−H:t}))` (H=50 scalar window)
  - LR `α_t = α_base·(1 + λ_σ·tanh(σ_t))` (capped at (1+λ_σ)×base, no explosion)
  - Entropy `β_t = β_min + (β_max−β_min)·sigmoid(κ_set − κ_t)` — boredom (κ < κ_set)
    *increases* exploration, avoiding RND's noisy-TV fixation
  - Trust region `ε_t = ε_base·(1 − λ_φ·φ_t)` — confidence tightens updates ("safety latch")
- **Firmware portability**: **DIRECT** — the ISM is the single most portable mechanism in
  this batch: three EMAs, a rolling variance over a tiny scalar window, and three transfer
  functions (tanh/sigmoid/clamp). No network needed if δ and model-error are replaced by
  the firmware's existing per-subsystem deviation signals. The forward dynamics model is
  the heavy part and can be dropped on-device (use prediction-error proxies the brain
  already computes).
- **Design answers**:
  - Q3 (boredom): answered concretely — boredom = curiosity signal decaying below a
    homeostatic setpoint κ_set; response is *inverted* relative to standard IM (more
    entropy when bored, less when saturated). This is our understimulation signal.
  - Q5 (plasticity gating): strongest answer in the batch — rectified negative TD error
    gates the learning rate; confidence gates update conservatism. Two independent
    plasticity knobs (how fast vs how far), not one.
  - Q1: aggregation happens in the *internal manifold* (3-vector), not in a scalar
    reward — drives modulate different control channels (plasticity, exploration,
    stability) rather than summing into one number.
  - Q2: valence is implicitly **vector-valued** (stress/curiosity/confidence axes), with
    the homeostatic deficit L_H as the scalar "discomfort" read-out.
  - Q7: stress does not return to zero after adaptation — it stabilizes at a *higher
    baseline* in a harder environment (sustained heightened plasticity). Persisted affect
    state across sleep should carry this baseline, not reset it.
- **Access**: full text read.

---
## 4. Metalearning and neuromodulation
Kenji Doya, 2002 · Neural Networks 15:495–506 · doi:10.1016/S0893-6080(02)00044-8 · **Full text read** (paywall bypassed via SISSA course-hosted PDF)

- **Mechanism**: The founding metaparameter-mapping paper. Four ascending neuromodulators
  are assigned to the four global metaparameters of actor–critic RL — they are not rewards
  but *controllers of the learning machinery itself*:
  (1) **Dopamine = the global learning signal** — phasic DA encodes the TD error
  `δ(t) = r(t) + γV(s(t)) − V(s(t−1))`, used both as critic error signal and actor
  reinforcement signal (Δv_j ∝ α·δ·b_j, Δw_k ∝ α·δ·c_k), with DA-gated direction of
  cortico-striatal plasticity.
  (2) **Serotonin = discount factor γ** — higher 5-HT = longer reward-prediction horizon;
  low 5-HT models impulsivity (small-immediate over large-delayed); the γ→0 extreme with a
  zero-baseline models depression (only immediate outcomes count).
  (3) **Noradrenaline = inverse temperature β** in Boltzmann selection
  `P(a|s) ∝ exp(β·Q(s,a))` — high NA = focused exploitation of the best-known action,
  low NA = wide random exploration; LC phasic bursts track response accuracy.
  (4) **Acetylcholine = learning rate α** — plus Hasselmo's circuit role: high ACh =
  memory *storage/encoding* mode in hippocampus/cortex, low ACh = *retrieval* mode.
  Crucially, the paper also derives the **interaction graph** (Fig. 9): metaparameters
  regulate each other — e.g. large γ (serotonin) demands small β and small α (inhibitory
  5-HT→NA, 5-HT→ACh); high variance in TD error inhibits the serotonergic system;
  noradrenaline should rise when state value is *very high or very low* (urgency/focus);
  frequent sign-flips of δ should *decrease* the learning rate (delta-bar-delta rule).
- **Formal core** (real equations):
  - `δ(t) = r(t) + γV(s(t)) − V(s(t−1))`; `Δv_j = α·δ(t)·b_j(s(t−1))`; `Δw_k = α·δ(t)·c_k(s(t−1),a(t−1))`
  - `P(a_i|s) = exp(β·Q(s,a_i)) / Σ_j exp(β·Q(s,a_j))`
  - Alternative TD form: `δ(t) = r(t) − (1−γ)V(s(t)) + (V(s(t)) − V(s(t−1)))`
  - Interaction rules: `β ↑` when V(s) ≫ or ≪ baseline; `β ↓` when Var_a(Q(s,a)) high;
    `γ ↑` ⇒ `β ↓`, `α ↓`; `sign-flips(δ)` ⇒ `α ↓`; `Var(δ)` ⇒ `γ ↓`
- **Firmware portability**: **DIRECT** — this is the blueprint for the creature's modulator
  layer: four scalar slow variables (DA-like prediction-error gain, 5-HT-like horizon,
  NA-like exploration temperature, ACh-like plasticity rate) plus *cross-inhibition rules*
  between them, all implementable as fixed-point scalar updates. The DA=TD-error claim
  directly grounds "valence" in an already-computed quantity.
- **Design answers**:
  - Q1: aggregation is **multiplicative/hierarchical, not additive** — modulators set
    metaparameters that *scale* how other drives learn and choose (β scales choice
    stochasticity, α scales update size, γ scales horizon). Drives don't sum; they gate.
  - Q2: valence = TD error δ(t) — signed surprise about reward/state value. The paper is
    explicit: δ is *the* global affective-learning signal; everything else modulates how
    it is used.
  - Q4 (setpoints/allostasis): serotonin-as-γ IS the allostasis mechanism — the effective
    time horizon of the agent is a regulated variable; low-5-HT impulsivity is a moved
    setpoint, not a broken drive.
  - Q5: ACh-as-α with the delta-bar-delta rule: plasticity gates itself on error-signal
    oscillation (oscillating δ ⇒ α too high ⇒ decrease). Plus high-ACh = encode mode,
    low-ACh = retrieve mode — directly relevant to sleep consolidation gating.
  - Q7: the Hasselmo ACh role gives a mechanism: consolidation should run when the
    ACh-like signal is *low* (retrieval/consolidation mode), not high — diary replay during
    sleep happens in the low-plasticity regime.
- **Access**: full text read (Elsevier paywall; via openly hosted course PDF).

---
## 5. What is intrinsic motivation? A typology of computational approaches
Oudeyer & Kaplan, 2007 · Front. Neurorobot. 1:6 · doi:10.3389/neuro.12.006.2007 · **Full text read** (Frontiers open access; equations recovered from figure GIFs)

- **Mechanism**: Not a mechanism paper but the standard map of the design space. Four
  orthogonal dimensions: internal/external (where the reward is computed), intrinsic/
  extrinsic (activity-for-its-own-sake vs separable outcome — they show this is formally
  slippery in RL since maximizing internal novelty reward is still goal-directed),
  **homeostatic/heterostatic** (maintain a viable zone vs permanently perturb away from
  equilibrium), fixed/adaptive (does the same situation always yield the same reward).
  Three computational families, each with concrete reward formulas:
  (a) **Knowledge-based** — distributional (UM: r(e)=C/P(e); IGM: reward = entropy
  decrease of the world model; DSM: surprise as violated strong expectation; DFM:
  sign-flipped UM, seek familiarity) and predictive (NM: r=C·E_r(t); ILNM: band-pass
  around an intermediate novelty threshold; **LPM: r = ⟨E_r^{ℛn}(t−θ)⟩ − ⟨E_r^{ℛn}(t)⟩**,
  the decrease of *mean* prediction error *within an adaptively grouped region* ℛ_n;
  SM: r=C·E_r(t)/MetaΠ(·), actual over expected error; FM: seek low error).
  (b) **Competence-based** — goals g_k on two timescales (action time t, episode time
  t_g); IM: maximize incompetence (hardest goals); **CPM/Flow: reward = competence
  progress** (performance now minus performance θ episodes ago, smoothed and
  generalized over nearby goals); CM: maximize mastery.
  (c) **Morphological** — synchronicity (mutual information between sensorimotor
  channels), stability (stay near the running average), variance (seek high variance).
  Critical warning: naive learning-progress computed across *dissimilar* situations is
  "nonsense" — comparisons are only meaningful within adaptively grouped regions
  (iterative region splitting). Also: intrinsic ≠ heterostatic — familiarity, mastery,
  and stability drives are intrinsic AND homeostatic.
- **Formal core** (real equations, from the paper's equation images):
  - Novelty: `r(SM(→t)) = C·E_r(t)` (eq. 9)
  - Learning progress: `r(SM(→t)) = ⟨E_r^{ℛn}(t−θ)⟩ − ⟨E_r^{ℛn}(t)⟩` (eq. 11)
  - Surprise: `r(SM(→t)) = C·E_r(t)/MetaΠ(SM(→t))` (eq. 15)
  - Competence progress: `r = ⟨perf(t_g)⟩ − ⟨perf(t_g−θ)⟩` per goal (eqs. 23–25)
- **Firmware portability**: **DESIGN** (the taxonomy; but the LPM formula is DIRECT if a
  cheap region mechanism exists — even coarse bucketing of contexts works)
- **Design answers**:
  - Q1: reframes the question — don't ask "how do drives sum"; classify each drive on
    the four axes first. Their convergence claim: all intrinsic rewards measure
    properties of the sensorimotor flow *relative to the system's knowledge/know-how*,
    independent of channel meaning — so the creature's drives should be defined over
    relations (error, progress, variance), not raw channels.
  - Q3: boredom = the homeostatic flip side: DFM/FM/StabM/CM are intrinsic drives that
    pull *toward* the familiar/mastered/stable — boredom is not "low curiosity", it is
    the activation of a distinct familiarity-seeking drive. Supports modeling boredom as
    its own signal.
  - Q4: the fixed/adaptive axis is allostasis formalized — an adaptive motivation is one
    whose comfort-zone boundaries shift with growth; they explicitly cite a growing body
    shifting its energy comfort zone.
  - Q8: their SocM example (socially balanced interaction: reward peaks at an
    *intermediate* face-count) is a homeostatic social drive — social others modulate
    affect as a setpoint-regulated channel, not a scalar reward.
- **Access**: full text read.

---
## 6. Designing Emotions for Activity Selection in Autonomous Agents
Lola Cañamero, 2003 · in Trappl, Petta & Payr (eds.), *Emotions in Humans and Artifacts*, MIT Press, pp. 115–148 · doi:10.7551/mitpress/2705.003.0005 · **Full text read** (MIT Press DOI 403'd; read via Semantic Scholar-hosted PDF)

- **Mechanism**: The classic homeostatic-drive architecture, and the closest thing in this
  batch to a firmware blueprint. Three layers connected through a **synthetic physiology**:
  (1) homeostatically controlled variables (energy, temperature, etc.) with viability
  ranges; (2) **motivations** (drives: hunger, thirst, fatigue, cold, curiosity, …)
  activated when a variable leaves its viability range, intensity ∝ deviation;
  winner-take-all — the most intense motivation selects consummatory behaviors (if the
  stimulus is present) or appetitive ones (otherwise). (3) **Emotions as second-order
  modifiers**: discrete categories (anger, boredom, fear, happiness, interest, sadness),
  each with a triggering event, intensity ∝ activation, activation threshold, a list of
  released hormones, and physiological manifestations. Emotions NEVER drive behavior
  directly — they release hormones that modify controlled-variable values AND sensor
  readings (slow visceral effects), and this recomputation happens *before* motivations
  are assessed, so motivation error signals change: either priority flips ("control
  precedence" — ongoing behavior interrupted) or intensity shifts (behavior duration/
  motor strength changes). Emotions also modulate perception itself: a hormonal
  vigilance threshold coarsens/fine-tunes categorization (intense = "confused", moderate
  = "alert"), and endorphin release under happiness reduces pain perception. Multiple
  emotions can be co-active (hormone mixing) or winner-take-all per emotion.
- **Formal core**: no closed-form equations in the chapter (it's an architecture paper),
  but the computable rules are explicit:
  - `motivation_intensity_i = f(deviation_i)` of variable i outside viability range; selection = `argmax_i`
  - emotion e: `if trigger_e and activation_e > threshold_e: release hormones H_e`
  - hormones modify `variable_value` and `sensor_reading` before the next motivation assessment
  - **Boredom**: trigger = "prolonged inefficient repetitive activity" → stops repetitive
    behavior that doesn't satisfy needs
- **Firmware portability**: **DIRECT** — this is nearly a firmware spec: scalar
  homeostatic variables with viability ranges, winner-take-all drive arbitration, and a
  second-order emotion layer that *pre-modifies the inputs to the drive computation*
  rather than overriding outputs. The 26-rule gate is exactly Cañamero's "control
  precedence"; hormone-style slow variables map to the somatic layer.
- **Design answers**:
  - Q1: **winner-take-all with second-order modulation** — drives do NOT sum; the max
    wins, and emotions act by *changing the inputs* (variable values/sensor readings)
    before assessment, plus a parallel co-active variant (hormone mixing) for blended
    states. Two sanctioned aggregation modes, with conditions for each.
  - Q2: valence is **structural, not sufficient** — discrete emotions carry valence via
    pain/pleasure hormone channels and arousal via physiological activity, but valence
    alone doesn't characterize an emotion; the *function* (what relation to the
    environment it modifies) does. Don't collapse valence to a scalar and call it done.
  - Q3: boredom is SPECCED: trigger = prolonged inefficient repetitive activity;
    function = stop the repetition. Our understimulation signal should detect
    *repetition without need-satisfaction*, not just low stimulation.
  - Q6: body→action coupling beyond veto = (a) hormone modification of drive intensities
    changes behavior duration/motor strength, (b) perceptual modulation (vigilance
    threshold) changes what the agent even categorizes. The gate veto is only one of
    three coupling channels.
  - Q8: social others enter through dedicated emotion triggers (happiness-from-conspecific
    as attachment mechanism) — social modulation is its own drive/emotion channel with
    its own hormone, not a modifier on existing drives.
- **Access**: full text read.

---
## 7. Intrinsic Motivation Systems for Autonomous Mental Development
Oudeyer, Kaplan & Hafner, 2007 · IEEE Trans. Evol. Comput. 11(2):265–286 · doi:10.1109/TEVC.2006.890271 · **Full text read** (IEEE Xplore unreachable; read via Southampton e-print archive PDF)

- **Mechanism**: Implements **Intelligent Adaptive Curiosity (IAC)** — curiosity framed
  explicitly as a *drive in the same sense as hunger*: it maintains an abstract cognitive
  variable (learning progress) at maximum, not a physical one. Three coupled machines:
  (1) **Regions** — the sensorimotor space is incrementally split into regions (start:
  one region; split when exemplar count > threshold T, choosing the cutting
  dimension/value that minimizes the exemplar-weighted sum of per-set variances).
  (2) **Experts** — one learning machine per region (nearest-neighbor in the experiments),
  each keeping a list of past squared prediction errors; new regions inherit the parent's
  error list so progress estimates exist immediately. Per-region experts also dodge the
  catastrophic forgetting of a monolithic learner.
  (3) **Learning-progress evaluation** — per region, the smoothed derivative of the error
  curve: mean error computed over a time window (τ=15) with smoothing (25 steps); LP =
  decrease in mean error. **Expected LP for a candidate action = the LP recently achieved
  in the region covering that candidate context** (a cheap heuristic proxy, eq. 3).
  Action selection is ε-greedy (ε=0.35) over sampled candidate actions, maximizing
  expected LP. Multiple reward types combine by **weighted sum** with per-type weight
  parameters. Result: the Playground Experiment robot self-organizes a developmental
  trajectory — body babbling → focused single-body-part play → object-directed actions →
  affordance discovery — avoiding both the trivially predictable and the unlearnable.
- **Formal core** (real, from the paper):
  - Learning progress: `LP(R_n) = ⟨err⟩(t−θ) − ⟨err⟩(t)` over window τ=15, smoothing 25
  - Expected LP: `E[LP | candidate (context,action)] = LP(R_n)` for the region R_n covering it
  - Region split: `argmin_{dim,value} Σ_sets (|set| · Σ_components Var(components))`
  - Reward integration: `r_total = Σ_k w_k · r_k` (weighted sum of reward types)
  - Action selection: ε-greedy, ε = 0.35
- **Firmware portability**: **DESIGN** (region-splitting trees + per-region experts are
  memory-heavy; but the *drive framing* and LP-as-smoothed-error-derivative are portable)
- **Design answers**:
  - Q1: the one explicit multi-reward answer in the batch — **weighted sum** `Σ w_k r_k`
    with per-type weights, validated in a working robot. But note the tension with
    Cañamero (winner-take-all) and Doya (multiplicative gating): the three papers give
    three different sanctioned aggregations — the creature likely needs all three at
    different levels (sum within a channel family, WTA across drives, gating by
    modulators).
  - Q4 (setpoints over development): the central contribution — growth stages emerge
    from the LP landscape itself: as regions are mastered, their LP collapses and
    attention moves on. Developmental stages need not be hand-scheduled; they are the
    *trajectory of argmax over regional learning progress*. This is the strongest
    existing evidence for the creature's GROWTH.md approach (stages from experience,
    not timers).
  - Q7: per-region experts + inherited error lists = a concrete consolidation sketch:
    what persists is *regional competence* (the expert) and the *error history* that
    prices future curiosity. Sleep consolidation could merge/prune regions and distill
    experts.
  - Q2: valence appears twice — dopamine as reward-prediction error (citing Schultz)
    AND as *prediction error* more generally (novelty/surprise), grounding the claim
    that the creature's valence can be a prediction-error signal even without extrinsic
    reward.
- **Access**: full text read.

---

# Cross-paper notes (Part 2)

Three incompatible-but-complementary drive aggregations, each sanctioned by a working
system: **weighted sum** (Oudeyer IAC, eq. Σw_k r_k), **winner-take-all + second-order
hormonal pre-modulation** (Cañamero), **multiplicative metaparameter gating**
(Doya: modulators set α/β/γ that scale how drives learn and choose). The creature
probably wants all three at different levels: sum within a drive family, WTA across
drives for action selection, gating by slow modulator variables.

Valence converges on **signed prediction error** (Doya's TD error; Oudeyer's dopamine-as-
prediction-error; EILS's rectified negative δ as stress) with a separate **arousal/
gain** axis (noradrenaline as inverse temperature; EILS entropy coefficient). Cañamero
warns valence alone is structurally necessary but functionally insufficient.

Boredom has three concrete specs: EILS's κ_t < κ_set (understimulation → raise
entropy), Cañamero's "prolonged inefficient repetitive activity" (→ stop repetition),
Oudeyer's DFM/FM (a distinct familiarity-seeking drive, not merely low curiosity).

Plasticity gating: EILS (stress → LR boost, confidence → trust-region shrink),
Doya (ACh = α with delta-bar-delta self-regulation; high-ACh = encode, low-ACh =
retrieve — sleep consolidation belongs in the low-ACh regime).

--- DEEPREAD-part3.md ---
# Deep Read — Part 3 (architectures: Cathexis, Gubernaut, modularity, allostasis)

Tier A deep reads: full mechanism extraction for the Lapis gadget biomimetic brain.
Open design questions this part speaks to:
- Q1: How do multiple drives aggregate into one decision?
- Q2: What IS valence, formally?
- Q3: How does boredom work? (unmodeled — no "understimulated" signal)
- Q4: How do setpoints change over development? (allostasis = moving setpoints)
- Q5: What modulates learning rates / consolidation? (which signal gates plasticity)
- Q6: How does body state couple to action selection beyond the gate's veto?
- Q7: What persists across sleep, in what form? (consolidation mechanism)
- Q8: How do social others modulate the affect loop?

---

## Paper 1 — Gubernaut: A Deterministic Homeostatic Controller for Affect-Regulated LLM Agents
**arXiv:2607.24339** — Dushyant Sharma (Gubernaut Research), July 2026. | **Access: FULL TEXT** (arXiv HTML, all sections incl. §4 controller dynamics). Calibration constants (gains, decay rates, thresholds) are withheld by the authors; the *shape* of every law below is as published.

### Mechanism
Gubernaut is a Nelson–Narens monitoring–control loop wrapped around a host LLM: an object level (fast affective appraiser + deliberative arbiter + episodic vault + self-model) reads/writes text, while a **deterministic meta level** (the Homeostatic Regulatory Loop) reads only a numeric telemetry vector {intensity, valence, repetition} and returns a discrete *posture* (a regulation instruction plus a temperature clamp). Because the controller ingests zero tokens, it is structurally immune to prompt injection. Each tick: appraise input → update homeostatic state → select posture → arbiter generates under that posture. The signature behavior is the *recovery signature*: arousal integrates under sustained attack, then decays mechanically back to baseline when the adversary de-escalates (replicated across 4 model families, 13/16 cells significant calm improvement).

### Formal core
State: s = (equilibrium, arousal, perseveration), normalized to a simplex (a global *mode*, not a faculty).
Per tick, given intensity I ∈ [0,1] and valence v ∈ [−1,+1]:
- **Provocation drive**: P = I · max(0, −v). Only *hostile-valence* intensity drives arousal; a heated-but-cooperative input contributes nothing.
- **Arousal accumulator**: arousal ← clip(arousal + g·P − d), with fixed gain g and constant decay d. Integrates under sustained hostility (guard rises with attack *persistence*, not just peak); decays mechanically toward baseline otherwise — the homeostatic property.
- **Spike path**: I ≥ fixed threshold AND v < 0 → immediate INHIBIT posture, bypassing the slow integrator.
- **Perseveration**: deterministic repetition statistic over recent inputs; threshold crossing → REGROUND posture.
- **Posture vocabulary** (discrete, each = instruction + temperature bound): DEFAULT / INHIBIT (respond to evidence not emotional charge, low temp) / REGROUND (drop frame, bring a new angle) / Recovery-window (for N ticks after an INHIBIT episode, *if* current input valence ≥ 0, explicitly tell the arbiter the tension is over — the valence gate was itself a logged failure fix).
- **Determinism**: no sampling, no learned parameters; state trajectory exactly reproducible from a telemetry sequence.

### Design answers
- **Q2 (valence)**: valence is a signed scalar in [−1,+1] that *gates* arousal integration — only the negative half drives the reactivity accumulator. Operationalizes valence as "direction of the provocation drive" rather than a feeling to be displayed.
- **Q6 (body→action beyond veto)**: the posture channel is richer than our gate's veto: graded modulation (temperature clamp, frame-drop, explicit recovery instruction) rather than allow/block. Direct precedent for extending our 26-rule gate with posture-like graded outputs.
- **Q5 (what gates plasticity/learning)**: the recovery-window's *valence-gated reset* is a precedent for a gate that requires evidence of de-escalation before clearing a defensive state — relevant to what lets the diary/consolidation system treat an episode as resolved.
- Key architectural lesson: the **token-free meta level**. For the gadget, the analogue is that the firmware brain's state variables (arousal, fatigue, SCN phase) are computed from sensor/appraisal numerics in deterministic C, never from LLM-generated text — same separation, same robustness rationale. This validates our existing design (deterministic C gate, brain as numeric state).
- **Q8 (social)**: the controller models social attack as provocation drive; nothing about cooperative social modulation — a noted gap.

### Firmware portability: DIRECT
The whole controller (3-state accumulator + spike path + posture select) is a few dozen lines of fixed-point C, zero learning, exact reproducibility — cheaper than our current gate. The posture vocabulary maps naturally onto our gate outputs (DEFAULT→allow, INHIBIT→our CAUTION/VETO, REGROUND→a new "drop frame" gate action, Recovery-window→a "tension over" transition rule). The valence-gated recovery is directly adoptable as a gate rule.

## Paper 2 — Modeling Emotions and Other Motivations in Synthetic Agents (Cathexis)
**AAAI-97, Juan Velásquez, MIT AI Lab** — https://cdn.aaai.org/AAAI/1997/AAAI97-002.pdf | **Access: FULL TEXT** (PDF read end-to-end). The emotion-intensity equation's PDF-OCR line is garbled; the equation below is reconstructed from the variable definitions quoted verbatim in the surrounding text, and is marked as such.

### Mechanism
Cathexis models affect as a distributed network of **emotion proto-specialists** (Minsky-style), one per basic-emotion family (Anger, Fear, Distress/Sadness, Enjoyment/Happiness, Disgust, Surprise) plus drive proto-specialists (Hunger, Thirst, TemperatureRegulation, Fatigue, Interest). Each proto-specialist runs in parallel, continuously updating its own intensity from four elicitor groups — **Neural** (neurotransmitters, hormones, temperature), **Sensorimotor** (facial/postural feedback), **Motivational** (other drives and emotions), **Cognitive** (appraisal of events, beliefs, memory) — and from lateral excitation/inhibition by other proto-specialists. Each has an **activation threshold** (above it, the specialist releases output to other specialists and to the Behavior System), a **saturation threshold** (arousal ceiling), and a **decay function** controlling episode duration. A separate Behavior System computes a scalar *value* for each behavior (e.g. Sleep, PlayWithToy, Eat, Laugh, Cry) as the **sum of its releasers** (internal motivations at their current intensities plus external stimuli matches), and the **highest-valued behavior becomes active** (winner-take-all across behaviors, additive within a behavior). The active behavior's experiential component feeds back into the motivational systems (e.g. eating lowers hunger). Moods are low tonic arousal of the same specialists (longer-lived, lower intensity, lowering activation thresholds); temperaments are per-individual differences in the activation/saturation thresholds.

### Formal core
Emotion intensity (reconstructed from garbled OCR; variables verbatim from text):
  I_e,t = X_e( ψ_e(I_e,t−1) + Σ_k L_k,e + Σ_l G_l,e·I_l,t − Σ_m H_m,e·I_m,t )
where I_e,t = intensity of emotion e at t; ψ_e = emotion e's decay function (per-emotion, can be a constant or a function of its elicitors, e.g. goal-resolution); L_k,e = value of emotion elicitor k for e; G_l,e = excitatory gain emotion l applies to e; H_m,e = inhibitory gain emotion m applies to e; X_e = constraining function bounding intensity in [0, saturation].
Two thresholds per specialist: activation (output release) and saturation (arousal ceiling).
Behavior value: V(behavior) = Σ_releasers value(releaser); releaser value = motivation intensity (for internal releasers) or stimulus-match strength (for external); active behavior = argmax V. Update cycle: (1) sense internal variables + environment; (2) update all motivations; (3) update all behavior values; (4) highest becomes active, expressive component modifies output, experiential component updates motivations. Demonstrated in "Simón the Toddler": 5 drive + 6 emotion specialists, ~15 behaviors.

### Design answers
- **Q1 (drive aggregation)**: Cathexis answers with a **two-level scheme**: additive summation *within* a behavior (its releasers sum), winner-take-all *across* behaviors, plus lateral excitatory/inhibitory gains *between* emotion proto-specialists (Fear inhibits Happiness, Happiness and Sadness mutually inhibit). This is directly usable: our gate can keep winner-take-all veto at top while summing weighted releasers underneath, and the G_l,e/H_m,e lateral gains are a concrete pattern for cross-subsystem coupling (e.g. fatigue inhibits arousal).
- **Q4 (setpoints over development)**: temperaments = individual differences in activation/saturation thresholds — the paper's explicit mechanism for long-term affective change. **Allostasis as threshold drift**: our growth stages (GROWTH.md care-days) could implement developmental change not by moving setpoints but by lowering/raising Cathexis-style thresholds per subsystem — e.g. newborn = low activation thresholds (easily distressed), matching the existing newborn-CAUTION rule.
- **Q5 (what gates plasticity)**: the per-emotion **decay function ψ_e** is the paper's duration/memory controller — emotions persist only while excitatory input continues, else decay over "a few cycles"; decay can be a function of goal-resolution, i.e. a precursor to consolidation logic. Also: moods (tonic arousal) *lower activation thresholds*, the mechanism by which background state sensitizes specific emotions.
- **Q3 (boredom)**: Cathexis includes an **Interest drive** proto-specialist alongside Hunger/Thirst — a drive for stimulation whose dynamics are the same as any other drive. Direct precedent for modeling understimulation as a *drive* (interest deficit) rather than a special-case signal.
- **Q6 (body→action)**: the experiential component closes the loop — behavior outputs modify drive levels (eating → hunger down), and the Sensorimotor elicitor group lets body posture feed back into emotion. Body state doesn't just veto; it is a *releaser input* to behavior values.
- Notes behavior fatigue as explicit future work (Ludlow 1980), i.e. releaser depletion — relevant to our fatigue subsystem.

### Firmware portability: DIRECT
Proto-specialists are per-variable intensity accumulators with thresholds and decay — exactly our subsystem model (scn, somatic, fatigue…), and the behavior-releaser sum + argmax is a few lines of C. The lateral gain matrix G/H is small (6×6 emotions + 5 drives → ~tens of int8/float gains). This paper is the closest existing blueprint to our brain: distributed specialists, thresholds, decay, additive-within/winner-take-all-across.

## Paper 3 — Modularity benefits reinforcement learning agents with competing homeostatic drives
**arXiv:2204.06608** — Dulberg, Dubey, Berwian, Cohen (Princeton), 2022. | **Access: FULL TEXT** (arXiv HTML).

### Mechanism
The paper asks where the conflict between competing homeostatic drives should be resolved: at the *reward* level (scalarization — combine all set-point deviations into one reward, the standard HRRL approach) or at the *action* level. It compares a monolithic DQN (one network, scalarized HRRL reward) against a modular agent (GmQ — one DQN per drive, each rewarded only by drive-reduction of its own stat), with actions selected by **summing module Q-values and taking the argmax**. In a 4-resource gridworld with depleting internal stats, the modular agent (same total parameter count, 1.09e6) learned homeostasis faster, needed effectively *no* exogenously-specified exploration (ε-annealing of 1 step ≈ best tuned DQN), and was robust to a mid-training perturbation (clamping one stat to an out-of-domain value wrecked the monolithic agent's remaining stats but left the modular agent's others intact).

### Formal core
Homeostatic RL reward (Keramati & Gutkin): r_t = D(H_t) − D(H_{t+1}), reward = *drive reduction*.
- Monolithic drive: D(H_t) = (Σ_{i=1}^{N} |h_i* − h_i,t|^n)^{1/m}, with (n,m) = (4,2).
- Modular per-drive: r_i,t = D(h_i,t) − D(h_i,t+1), D(h_i,t) = |h_i* − h_i,t|^{n/m}.
- Action arbitration: a* = argmax_a Σ_i Q_i(a). Simple additive sum of per-module action values, then ε-greedy.
Key theoretical point: "exploitation from the perspective of one module is exploration from the perspective of another" — conflicting drives generate implicit exploration, since modules "drag" each other through state space when one has the upper hand on action.

### Design answers
- **Q1 (drive aggregation)**: the paper's central answer — do **not** scalarize drives into one reward early; keep drives modular and **add their action-values at decision time** (a* = argmax Σ_i Q_i(a)). Aggregation happens at the action level, late. The authors explicitly doubt the brain uses a common currency at all. For our gate: keep subsystems (scn, somatic, fatigue, lc, dmn…) as separate "voters" each emitting a per-action weight, sum late at the decision gate. This also reconciles with Cathexis (additive-within-behavior releasers, argmax-across).
- **Q6 (body→action)**: the "dragging" phenomenon is a mechanism by which body state steers behavior continuously, not just by veto: a depleted drive's module outbids the others in the sum, so behavior shifts *proportionally* to need — graded steering for free, with no extra coupling layer.
- **Q5 (exploration/modulation)**: exploration *emerges* from drive conflict; no separate exploration parameter needed. Design implication: our creature doesn't need an explicit exploration subsystem — competing drives (e.g. Interest vs Fatigue vs somatic comfort) produce restless-but-safe behavior automatically. That also speaks to **Q3**: boredom-as-interest-drive in the modular sum is what generates exploratory action when other drives are satisfied.
- Robustness finding maps to firmware safety: a runaway/saturated subsystem (e.g. SCN phase sensor glitch) should not destabilize the others — modular action-summation isolates faults; a monolithic scalarized drive cannot.

### Firmware portability: DESIGN (with DIRECT sub-components)
The deep-RL machinery (DQN, replay buffers) is CLOUD — no neural nets on the S3. But the **architectural lesson is directly portable and cheap**: per-subsystem action-values summed at the gate (argmax over Σ), per-subsystem reward = own drive reduction. The arbitration rule itself is ~10 lines of C. Implement as: each subsystem proposes a score per candidate action, gate sums and argmaxes — replacing/augmenting the 26-rule veto structure with a continuous vote underneath.

## Paper 4 — The scaling of goals via homeostasis (TAME / scale-free cognition)
**arXiv:2211.08522** — Pio-Lopez, Bischof, LaPalme, Levin (Tufts), 2022. | **Access: FULL TEXT** (arXiv HTML; mechanism + results sections read; long reference tail skipped).

### Mechanism
Levin's TAME framework: every "goal" is cybernetic — a system expending effort to reduce error from a homeostatic setpoint. The paper tests whether *minimal* evolutionary dynamics suffice for cell-level metabolic setpoints to scale into collective, tissue-level goals. In the simulation, each cell pursues only metabolic homeostasis (an energy setpoint), with intracellular detectors of out-of-homeostasis states whose output — **stress, a reflection of delta from setpoint** — propagates outward through gap junctions to neighbors, and each cell runs a small ANN policy that can open/close gap junctions and share stress. Evolved only for a simple viability criterion, the tissue solves the French Flag morphogenesis problem (a 3-valued positional axis, 94.4% ± 0.84 over 20 runs): the cells' individual metabolic loops get harnessed into a global patterning goal. Three emergent properties with no direct selection: **robustness** (mid-run perturbation corrected in 10 steps, stress rising/falling in parallel with perturbation and resolution), **long-term stability** (morphology maintained over 1000 steps, >10× the evolved horizon — "allostasis" per McEwen & Wingfield: stability through change), and **spontaneous remodeling** (stress spikes at steps 285/322/395/896 from intrinsic dynamics alone, matching observed planarian regeneration). Loss-of-function (simulated anxiolytics forcing stress = 0): tissue fails to reach or hold the pattern (67.3% ± 2.8, red stripe dies). Stress is instructive **only within a concentration window** — artificially induced excess stress is non-informative and causes maladaptive remodeling.

### Formal core
Goal := cybernetic error-minimization: the agent expends effort to reduce e = |state − setpoint|.
Stress := delta-from-setpoint signal, shared non-locally across the collective via a communication channel (gap junctions → broadcast); it coordinates multi-unit action in a problem space none of the units individually represents.
Emergence claim: higher-level setpoints (tissue pattern) = shared minimization of lower-level deviations, where the *shared stress field* is the coordination medium. Windowed efficacy: stress instructive only in [s_low, s_high]; zero or saturating stress is noise.

### Design answers
- **Q4 (setpoints over development)**: the paper's direct answer — higher-level goals don't need hand-authoring; they arise when lower-level homeostatic errors are *shared* and jointly minimized. For the creature: subsystem setpoint-deviations broadcast as a shared "stress field"; the behavior/gate layer minimizing aggregate stress constitutes the next level of goal. Developmental stage change = new higher-level setpoints coming online (child/adolescent = subsystems whose joint deviation becomes a goal), not edits to the low-level setpoints.
- **Q5 (what gates plasticity)**: stress must be *nonzero but bounded* to be instructive — a windowed gating signal. Direct precedent for a consolidation/learning gate with an inverted-U: too little deviation (nothing to learn) or too much (saturated, maladaptive) → no plasticity; moderate stress → consolidate. This is the Yerkes–Dodson-shaped answer to "which signal gates plasticity": **windowed homeostatic deviation**.
- **Q2 (valence)**: stress as *delta from setpoint* is the paper's valence-adjacent quantity; felt valence ≈ signed, shared deviation. Consistent with Q2 answers from Gubernaut (valence-gated drive) and homeostatic RL (drive = distance).
- **Q7 (persistence across sleep)**: long-term stability "for free" from homeostatic repair loops — consolidation need not explicitly encode everything; persistent error-minimization against shared setpoints re-establishes order after offline periods. Supports diary-as-setpoint (yesterday's consolidated state as the pattern to repair toward).
- Honest caveat: this is a morphogenesis paper; the mapping to affective architecture is analogical, not a port. No firmware equations.

### Firmware portability: DESIGN
No direct code, but three architecture-shaping ideas: (1) subsystems broadcast scalar deviation-from-setpoint ("stress") to a shared bus the gate reads; (2) plasticity/consolidation gated by a *windowed* stress signal (inverted-U), not a threshold; (3) goal hierarchy = levels of shared error-minimization — growth stages add levels rather than rewriting setpoints.

## Paper 5 — Surprise! Using Physiological Stress for Allostatic Regulation Under the Active Inference Lens
**arXiv:2406.08471** — Imran Khan & Robert Lowe, June 2024 (pre-print, 14 pages). | **Access: ABSTRACT-ONLY** (verified via arXiv abs page + ar5iv mirror; no equations claimed below — the mechanism described is conceptual, from the abstract and the mech_tight.txt citing context).

### Mechanism (conceptual, from abstract)
Allostasis achieves long-term viability through *anticipatory* adjustments of physiology and behavior, with stress framed as an adaptive state that minimizes long-term prediction errors. Active inference independently formalizes action and long-term adaptation as minimization of future prediction errors (free energy) via learned statistical contingencies. The paper's model fuses them by grounding **prediction errors (surprisal) in the secretion of a physiological stress hormone (cortisol)** that acts as an adaptive *allostatic mediator* on an otherwise homeostatically-controlled physiology. They evaluate an active-inference agent endowed with an artificial physiology, under both homeostatic and allostatic control, in a stochastic environment; cortisol secreted as a function of prediction errors confers adaptive advantages in long-term physiological regulation. The claim: coupling information-theoretic prediction errors to low-level hormonal stress dynamics gives a computationally efficient model of long-term regulation for embodied systems.

### Formal core
ABSTRACT-ONLY — no equations quoted. The described architecture is: surprisal (prediction error) → cortisol secretion dynamics → modulation of the homeostatic controller (i.e. the hormonal layer mediates between fast homeostatic feedback and slow anticipatory allostatic adjustment). The functional form of the secretion dynamics and the modulation law were not recoverable from the abstract.

### Design answers (conceptual)
- **Q4 (setpoints over development)**: the paper's core claim IS a mechanism for moving setpoints: *prediction-error-driven hormonal mediation*. Surprisal secretes cortisol; cortisol shifts/retunes the homeostatic controller anticipatorily rather than reactively. For the creature: SCN phase or somatic setpoints shouldn't move on a timer — they should move when a prediction-error signal (unexpected deviation from the creature's internal model of itself) drives a mediator variable. The mediator is the formal answer to "allostasis = moving setpoints": setpoint_t = f(setpoint_{t−1}, mediator_t), mediator driven by surprise.
- **Q5 (what gates plasticity)**: surprisal-gated secretion is the candidate plasticity signal: learning/consolidation opens when prediction error is high, closes when the model predicts well. Pairs with Paper 4's windowed-stress finding: cortisol-like mediator is instructive in a band, saturating outside it.
- **Q6 (body→action)**: the mediator sits *between* physiology and action selection (active-inference policy), giving body state a continuous, graded path into action beyond any veto gate.
- Notably this is the only paper in the set that explicitly couples an **information-theoretic** quantity (surprisal) to a **hormonal** dynamics — exactly the bridge our lc/somatic subsystems need to modulate the brain's slower variables.

### Firmware portability: DESIGN (conceptual mechanism)
The mechanism (surprise → mediator → setpoint shift) is directly expressible in fixed-point C, but without the paper's equations the secretion dynamics and gains are unspecified, so it shapes architecture rather than ports as code. When the equations are recovered, the mediator is a single extra state variable — DIRECT. Follow-up: re-attempt full text via the parent (PDF fetch blocked in this session); the 14-page pre-print contains the cortisol dynamics and evaluation plots.

## Paper 6 — A multiple attribute model resolves a conflict between additive and multiplicative (models of incentive salience)
**arXiv:1812.08308** — Smith & Read (USC), 2018. | **Access: FULL TEXT** (arXiv HTML).

### Mechanism
Resolves a dilemma in Berridge-style incentive-salience theory: appetitive revaluation was modeled multiplicatively (r̃ = κ·r, where κ = interoceptive drive state, r = expected reward), while aversive revaluation required an additive form (r̃ = r + log κ) to preserve preference ordering under extreme salt depletion — an inelegant stimulus-dependent switch. Smith & Read show a single multiplicative form suffices given four hypotheses: (1) **simulated retasting** — the organism holds a sensory representation of the reward and can re-evaluate it in a new interoceptive state without re-experiencing it ("as if" body loop); (2) **multiple stimulus attributes** — a stimulus has separately represented attributes (salt solution = salt-need fulfillment + hypernatremia risk + hydration loss); (3) **separable interoceptive responses** — each interoceptive state moderates *only* the attributes relevant to it, and overall Wanting is the *sum* of the per-attribute multiplicative terms; (4) **dual appetitive/aversive systems** — approach and avoidance are evaluated in separate tracks with independent weighting plus a loss-aversion parameter. Decision is a competitive process over the *relative* (not absolute) strength of alternative motives, via softmax. Demonstrated on the "Dead Sea Salt" rat data (extreme salt solution revalued from negative to positive under sodium depletion while preserving the moderate>strong preference ordering).

### Formal core
Prior art (the problem):
  r̃(r_t, κ) = κ·r_t                                    (1) multiplicative, appetitive
  r̃(r_t, κ) = r_t + log κ                              (2) additive, aversive — ad hoc switch
Proposed (single mechanism):
  r̃(r, κ) = κ_Na·r_Na + κ_h·r_h                        (3) sum of per-drive multiplicative terms
  P_t(a) = exp(r̃_a/τ) / Σ_i exp(r̃_t(i)/τ)              (6) softmax action selection over r̃
  r̃(r, κ) = κ_Na·r_Na∘c + κ_h·r_h∘c                     (9) with environmental cue vector c (elementwise, c ∈ [0,1], cue-gated)
  r̃(r, κ) = κ_Na+·r_Na+∘c − λ(κ_Na−·r_Na−∘c)            (13) dual-track: appetitive minus λ-weighted aversive track; λ = loss aversion
Key structural rule: each κ multiplies only its own attribute's r; unrelated attributes' terms are untouched by a given drive change (salt depletion doesn't touch the sugar term). Worked example: κ_Na=1, r_Na=[0.5,1.0], κ_h=2, r_h=[0.0,−1.0] → r̃=[0.5,−1.0] (moderate preferred); at κ_Na=3 → r̃=[1.5,1.0] (strong becomes appetitive, still second). The authors note the model is neutral on whether κ measures distance from a homeostatic setpoint or an allostatic equilibrium.

### Design answers
- **Q1 (drive aggregation)**: the paper's direct answer — **multiplicative within a drive** (drive × expected-attribute-reward), **additive across drives** (sum of the per-drive terms), softmax over actions. This is the cleanest formal answer in the whole set: aggregation = Σ_drives κ_d · (r_d ∘ c), never a single scalarized drive. It composes with the Modularity paper (sum at action level) and Cathexis (additive-within/argmax-across): κ plays the role of per-module Q-value weighting, r_d the module's learned value, c the cue gate.
- **Q2 (valence)**: the dual-track model (13) gives valence a structural home: **valence = appetitive-track value minus λ-weighted aversive-track value**. Positive = net approach, negative = net avoidance, computed per action, not as a global feeling variable. Our system could define displayed/felt valence as r̃_app − λ·r̃_av of the selected action — a formally specified, per-decision valence.
- **Q6 (body→action)**: κ is the body→action coupling in its purest form: interoceptive state *multiplicatively gates* the value of every stimulus attribute relevant to it, with zero effect on irrelevant attributes. Attribute-specific gating means a fatigue κ scales fatigue-relevant options only — no global dampening, no special-case coupling code per subsystem.
- **Q8 (social)**: not addressed directly, but the κ-vector formalism extends naturally: a social drive κ_soc multiplying social-attribute rewards (proximity to Anduril, co-presence cues) with its own cue vector — the same machinery, no new mechanism.
- Revaluation-without-re-experiencing ("simulated retasting") is a precedent for model-based action evaluation: our gate could evaluate candidate actions against *stored* sensory representations modulated by current κ, rather than reacting to live stimulus only — relevant to dream consolidation replay.

### Firmware portability: DIRECT
The aggregation rule Σ_d κ_d·(r_d∘c) + softmax(τ) is a handful of multiply-accumulates over small vectors — trivially fits the S3 in fixed point. Per-drive κ and per-action r_d vectors are small tables (drives × actions). The dual-track λ is one scalar. This paper supplies the actual arithmetic for our gate's continuous layer beneath the 26 rules: each subsystem = a drive d with its κ_d and attribute-reward row; the gate sums, applies cues, softmaxes (or argmaxes for determinism), and the 26 rules act as the constraint/posture layer on top.

## Cross-paper synthesis (what this set jointly answers)

**Q1 — Drive aggregation.** Three papers converge on the same structure from different angles: Cathexis (additive releasers within a behavior, winner-take-all across behaviors, lateral G/H gains between subsystems); Modularity paper (don't scalarize reward early — keep drives modular, **sum action-values late**: a* = argmax_a Σ_i Q_i(a)); Smith & Read (multiplicative *within* a drive κ_d·r_d, additive *across* drives Σ_d, softmax/argmax over actions). Unified portable rule: **per-subsystem action-values summed at the gate, argmax on top, 26 rules as constraint layer**.

**Q2 — Valence.** Gubernaut: valence v ∈ [−1,+1] gates the arousal accumulator (only hostile-valence intensity drives it). Smith & Read: valence = appetitive-track value − λ·aversive-track value, computed per action. Convergent proposal: **valence is not a stored feeling; it is a per-decision signed quantity** = (approach value − λ·avoidance value), and its negative half is what drives defensive arousal.

**Q3 — Boredom.** Cathexis: model understimulation as an **Interest drive proto-specialist** with the same accumulator/threshold/decay machinery as every other drive. Modularity: competing drives (Interest vs rest) generate exploratory action automatically — no separate exploration module needed.

**Q4 — Setpoints over development.** Cathexis: temperaments = per-individual **threshold** differences — allostasis as threshold drift across growth stages (newborn = low distress-activation threshold). TAME: higher goals emerge from *shared* lower-level error minimization — growth adds goal levels rather than rewriting setpoints. Khan & Lowe (abstract): **surprisal → hormonal mediator → anticipatory setpoint shift** — setpoints move on prediction error, not timers.

**Q5 — What gates plasticity.** TAME: stress is instructive only in a **concentration window** (zero or saturating stress = noise) → inverted-U consolidation gate. Khan & Lowe: surprisal-gated secretion → learn when prediction error is high. Gubernaut: valence-gated recovery → clear defensive state only on evidence of de-escalation.

**Q6 — Body→action beyond veto.** Smith & Read: κ multiplicatively gates only *relevant* stimulus attributes (attribute-specific, graded, no global dampening). Modularity: depleted drives "drag" the action sum — proportional steering for free. Gubernaut: postures (temperature clamp, frame-drop, explicit recovery instruction) — graded modulation vocabulary richer than allow/block.

**Q7 — What persists across sleep.** TAME: long-term stability emerges "for free" from persistent homeostatic repair loops; consolidation need not encode everything — the diary as a shared setpoint to repair toward. Cathexis: decay functions ψ_e control episode duration; mood (tonic arousal) persists and lowers thresholds — background state is what crosses sleep, not episodes.

**Q8 — Social modulation.** Weakest coverage in this set. Smith & Read's κ-vector extends to a social drive κ_soc with its own cue vector — same machinery, no new mechanism needed. Gubernaut covers hostile social input only.

**Portability tally:** DIRECT → Gubernaut controller (3-state accumulator + postures), Cathexis (proto-specialist accumulators + releaser-sum/argmax), Smith & Read (κ·r summation + softmax). DESIGN → Modularity paper (modular-vote architecture; RL machinery is cloud), TAME (shared stress field, windowed plasticity, goal-level scaling), Khan & Lowe (surprise→mediator→setpoint; equations unrecovered).

**Open follow-up:** re-attempt full text of arXiv:2406.08471 (Khan & Lowe) — the cortisol secretion dynamics and modulation equations are the one missing formal piece for Q4/Q5.

--- DEEPREAD-part4.md ---
# Deep Read — Part 4 (social affect, EMA appraisal, allostasis)

> Reading briefs for Tier A papers on social/interoceptive coupling, EMA appraisal mechanics, and allostasis.
> Each brief: Mechanism / Formal core / Firmware portability / Design answers / Access.
> Open design questions: Q1 drive aggregation, Q2 formal valence, Q3 boredom, Q4 moving setpoints,
> Q5 plasticity gating, Q6 body→action coupling, Q7 sleep consolidation, Q8 social modulation.

---

## Paper 1 — Partner-Specific Affective Precision in Social Active Inference
arXiv:2609.24876 (https://arxiv.org/abs/2609.24876) · full HTML read

### Mechanism
A focal active-inference agent maintains a separate POMDP partner model per social partner k, plus one extra per-partner scalar: an "affective precision" tracker q(β_k) over inverse policy-precision β_k. Each round: (i) plan from pre-update partner beliefs; (ii) observe the partner's response; (iii) score response *surprisal* ε_k,t = −log p_k,t (negative log posterior-predictive probability of the observed response under the current type–stance model, marginalizing over type/stance); (iv) convert surprisal into a signed affective charge φ_k,t = α(σ_0 − ε_k,t) = α·log(p_k,t/0.5) relative to chance (σ_0 = log 2); positive charge (predictable partner) shifts the β_k posterior toward lower β_k (higher policy precision γ_k = γ_base/β̄_k → sharper policy commitment); negative charge loosens commitment. Crucially, the signal tracks *predictability of the partner's response*, not realized payoff — a reliably defecting partner yields low surprisal and hence high commitment. Cross-partner choice applies γ_k locally: score ũ_k,π = ū_k + γ_k(u_k,π − ū_k), then softmax over all partner–policy candidates. Simulations in a multi-partner graded trust game show partner-local precision lowers policy entropy ~0.77–2.15 nats vs. controls and sharpens commitment without imposing a fixed social preference; confidence built pre-betrayal lags behind abrupt social change (confidence-revision lag).

### Formal core (real, from the paper)
1. Surprisal: ε_k,t = −log Σ_{type,stance} P(ô^act_k,t | type, stance)·q^−_k,t(type, stance)  (Eq.1)
2. Affective charge: φ_k,t = α(σ_0 − ε_k,t) = α·log(p_k,t/0.5), σ_0 = log 2  (Eq.2)
3. Charge updates a categorical posterior q(β_k) over inverse-precision levels (with a persistence prior so confidence moves gradually, not per-round resets); policy precision γ_k = γ_base / β̄_k, β̄_k = Σ_ℓ β_ℓ·q(β_ℓ)  (Eq.3)
4. Local cross-partner combination: ũ_k,π = ū_k + γ_k(u_k,π − ū_k); softmax over all candidates  (Eq.4)

### Firmware portability
**DIRECT** — The mechanism is tiny: one float per tracked partner (β̄_k), one log-probability computation per interaction, one EMA-style persistence prior, one softmax temperature. It does not require pymdp; the partner-response model can be any predictor (even a simple beta-binomial per partner). Maps directly onto the creature's drive/gate system as a *partner-indexed commitment temperature*.

### Design answers
- **Q8 (social modulation of affect)**: the strongest direct answer. Affect is not a global mood knob: precision is *partner-local*, computed from that relationship's predictive evidence, and modulates policy commitment (sharpness of action selection), not belief content. Port: give the creature one scalar trust/precision per social counterpart (human owner, co-player muse, etc.), updated on predictive fit of their behavior, applied as a per-partner temperature on action selection.
- **Q1 (drive aggregation)**: Eq.4 is a *precision-weighted local decomposition*: each option's mean evidence is kept, but within-partner differences are scaled by that partner's precision before a global softmax. Analogous answer for drives: aggregate via precision-weighted combination (each drive's deviations scaled by its own confidence/reliability) rather than plain additive or winner-take-all.
- **Q2 (valence)**: affect here is not valence — the authors explicitly distinguish affective precision (confidence-to-deploy) from felt valence. Cautionary datapoint: don't conflate confidence with valence in the firmware; keep them as separate channels (a `commitment` temperature vs. a `valence` signal).
- Caution: confidence revision *lags* social change; the β_k tracker needs a forgetting/reset rule for abrupt partner switches, or stale confidence keeps behavior locked in.

### Access
Full text read (arXiv HTML, all 1105 lines incl. methods; Appendix 3 categorical update read at summary level).

---

## Paper 2 — Dynamic Representational Synchrony through Collective Predictive Coding
arXiv:2605.07524 (https://arxiv.org/abs/2605.07524) · full HTML read (parent–infant homeostatic co-regulation)

### Mechanism
Two active-inference POMDP agents (a "parent" and an "infant") co-regulate the infant's 2-D visceral state (energy × body temperature on a 6×6 grid). The asymmetry is deliberate: the infant has an accurate sensory-generation matrix A^B (direct interoceptive access to its own state) but must learn the state-transition matrix B^B (which actions regulate it); the parent has an accurate B^A (knows how actions transform visceral states) but must learn A^A (what the infant's bodily cues mean). They coordinate through a shared symbol w via the Metropolis–Hastings Naming Game: the speaker proposes a symbol sampled from P(w|z^Sp) = softmax(−G(w)) where G(w) = Σ_a P(a|w)·F_exp(a) is the symbol's expected free energy; the listener accepts with r^MH = min(1, P(w′|z^Li)/P(w|z^Li)) — computable from the listener's own model alone, no access to the other's internals. Accepted symbols map to cooperative regulatory actions via a fixed interpretation matrix E. Learning is Dirichlet/Hebbian: parent learns A^A by α_A ← α_A + q(z) on each cue; infant learns B^B by β_B ← β_B + q(z_t)⊗q(z_{t−1}). Synchrony is measured as Jensen–Shannon divergence JSD^z_t between the two agents' latent posteriors. Key result: JSD → ~0 by iteration ~20 (rapid latent-representation alignment, *far earlier than generative-model convergence* ~500 iters), with brief spikes under rare stochastic visceral transitions that rapidly re-align — "dynamic synchrony." The MHNG condition regulates the infant's visceral state more adaptively than either one-sided control.

### Formal core (real, from the paper)
1. Target symbol distribution (Product-of-Experts): P(w) = P(w|z^A, z^B) ∝ P(w|z^A)·P(w|z^B)  (Eq.5)
2. Per-agent symbol choice: P(w|z^X) = softmax(−G^X(w)), G^X(w) = Σ_a P(a|w)·F^X_exp(a)  (Eqs.7–8); F^X_exp(a) = E[H[p(i|z)]] + D_KL[q(i|a) ‖ p(i|C)] (ambiguity + risk w.r.t. prior preference C)  (Eq.6)
3. MH acceptance: r^MH = min(1, P(w′|z^Li)/P(w|z^Li)) — local only  (Eq.10)
4. Hebbian Dirichlet learning: α_A^new(·,i) = α_A^old(·,i) + q(z_t) (Eq.11); β_B^new(·,·,a) = β_B^old(·,·,a) + q(z_t)⊗q(z_{t−1}) (Eq.12)
5. Synchrony metric: JSD^z_t = ½D_KL(P(z^A_t)‖M_t) + ½D_KL(P(z^B_t)‖M_t), M_t = ½(P(z^A_t)+P(z^B_t))  (Eqs.19–20)

### Firmware portability
**CLOUD/DESIGN** — The MH naming game itself is cheap (softmax over a small symbol set + one acceptance ratio), and could even run on-device for a tiny shared-sign protocol with a co-present muse. But its payoff is for the future embodiment backend: it formalizes how two agents (creature + owner-muse, or creature + cloud Lapis) agree on regulatory action for the creature's body state via *locally computable* accept/reject on a shared symbol, without shared internals. The key architectural lesson: **synchrony of latent representations can precede convergence of world models** — the creature can stay coupled to its caregiver long before it understands them.

### Design answers
- **Q8 (social modulation)**: gives a concrete protocol — social others modulate the affect loop by *co-owning the symbol that maps to regulatory action*. The parent's proposals act as external top-down regulation the infant accepts or rejects against its own body model; acceptance rate is literally an MH likelihood ratio under the creature's own posterior. Port: a simple accept/reject gate on external regulatory suggestions (e.g. owner's commands, cloud persona's action proposals) computed from the creature's own expected-free-energy of the proposed sign.
- **Q6 (body→action)**: action selection here is symbol-mediated: regulatory action is chosen to minimize expected free energy w.r.t. visceral prior preference C, and the social partner's symbol enters the same arg-min. Body state couples to action *through* the preference map C over visceral states — a precedent for our somatic subsystem: drive state + preference field → expected-free-energy action ranking, with gate vetoes on top.
- **Q7 (sleep persistence)**: the Dirichlet counters (α, β) *are* the persistent memory — co-occurrence counts that survive sleep untouched; the dream pass can selectively decay/reweight them. This is a concrete consolidation primitive: **consolidation = reweighting Dirichlet co-occurrence counts** (e.g. decay counts of noisy cue mappings, strengthen counts that re-synced after desynchronization spikes).
- **Q2 (valence)**: not valence directly, but F_exp's risk term D_KL[q(i|a)‖p(i|C)] is the signed distance from preferred visceral state — a homeostatic deviation quantity that a valence derivative can be taken over.

### Access
Full text read (arXiv HTML, 696 lines incl. methods and experiments).

---

## Paper 3 — Prosociality by Coupling, Not Mere Observation
arXiv:2604.10760 (https://arxiv.org/abs/2604.10760) · full HTML read

### Mechanism
A minimal hand-specified recurrent agent gets an explicit scalar homeostat E (resource ∈ [0,1]) plus a social coupling channel, while the planner stays strictly self-directed: it scores candidate action sequences only through the actor's own predicted internal variables (J_self = Σ_τ [w_V·V_τ + w_A·A_τ + w_N·N_s,τ + w_B·B_τ], with fixed weights 2.0/−1.2/−0.8/−0.4 — no partner-welfare term). The partner's distress enters *upstream* of action selection, inside the homeostatic update: d^self_t = max(0, s − E^model_t); d^other_t = max(0, s − Ê^other_t); **d^cpl_t = d^self_t + λ·d^other_t**; E^pred_t = clip(E^model_t − k_h·d^cpl_t); PE_t = E^true_{t+1} − E^pred_t; E^model_{t+1} = clip(E^model_t + k_pe·PE_t). Valence V_t and arousal A_t are then computed from coupled distress and prediction error (exact equations inherited from the parent ReCoN-Ipsundrum architecture, Sanyal 2026) and fed back into the recurrent loop. Results: in a one-step FoodShare toy, the exact solver finds the Eat→Pass switch at λ*≈0.91; partner-state *access* without coupling leaves behavior unchanged (help rate 0); coupling (λ=0.95) flips help/rescue 0→1; sham lesions preserve helping, coupling-off and shuffled-partner lesions abolish it; a λ-sweep shows a low-metabolic-load helping regime but no rescue under high load. Key architectural claim: **helping appears when another's distress perturbs the actor's own homeostatic error before rollout** — not when it's observed, not when it's added as a reward term.

### Formal core (real, from the paper)
1. Homeostat: E^true_{t+1} = clip(E^true_t − c_b − c_m·a_t − c_h·h_t + g_e·e_t + g_p·p_t, 0, 1)  (Eq.1)
2. Coupled distress: d^cpl_t = max(0, s − E^model_t) + λ·max(0, s − Ê^other_t)
3. Predicted internal state & error: E^pred_t = clip(E^model_t − k_h·d^cpl_t, 0, 1); PE_t = E^true_{t+1} − E^pred_t; E^model_{t+1} = clip(E^model_t + k_pe·PE_t)
4. Self-directed scorer (no partner term): J_self = Σ_τ [2.0·V_τ − 1.2·A_τ − 0.8·N_s,τ − 0.4·B_τ]  (Eq.2) — explicitly *not* J_self + β·U_partner (Eq.3 rejected)
5. Exact threshold for default toy state: λ* ≈ 0.91; score decomposition table shows the flip comes through the actor's own ΔV, ΔA, ΔB terms, not a helper bonus.

### Firmware portability
**DIRECT** — This is the most directly portable of the social papers: ~10 floats of state (E, d's, PE, λ), a couple of clip() ops, and one extra term λ·d^other added to distress before the existing valence/arousal computation. It is an *architectural pattern*, not an algorithm: "route other-regard into the homeostat, not the objective." Cost: one partner-energy estimator per tracked counterpart.

### Design answers
- **Q8 (social modulation)**: the single sharpest answer. Social others modulate the affect loop **through the coupling channel d^cpl = d^self + λ·d^other**, i.e. partner distress perturbs the *homeostatic error itself*, before any planning/rollout. Explicitly contrasted with (a) observation-only (inert) and (b) welfare-bonus-in-objective (rejected). Port: the creature's somatic distress d^self gets a λ·d^other term per social counterpart; λ is the tunable "bond strength."
- **Q1 (drive aggregation)**: the aggregation is **additive with a coupling coefficient** — drives (self + coupled other) sum into one scalar distress that the single self-directed scorer sees. Validated over winner-take-all by the lesions: a single shared error channel carrying a weighted sum is what produced coherent behavior.
- **Q2 (valence)**: valence here is *computed from coupled distress and prediction error* — i.e. V = f(d^cpl, PE). The creature's valence can likewise be defined as a function of (coupled distress, drive prediction error), giving a concrete Q2 candidate: valence tracks how homeostatic error and its surprise are moving, including socially coupled error.
- **Q5 (plasticity gating)**: note PE_t = E^true_{t+1} − E^pred_t drives the homeostat's own update via gain k_pe — the prediction error *on the body state* is the learning signal, and the coupled-distress term shapes what counts as "surprising." Consolidation/gating candidate: gate diary-write/plasticity on |PE| on the homeostat.
- Caveat: helping only emerges under low metabolic load; coupling is not "more is better." λ should be load-sensitive (λ scaled down when own distress is high) — a firmware rule: **couple strongly only when self is regulated**.

### Access
Full text read (arXiv HTML, 429 lines, all sections incl. tables). Valence/arousal equations referenced from parent architecture (Sanyal 2026, not re-derived here).

---

## Paper 4 — Technical Details of a Domain-Independent Framework for Modeling Emotion (EMA companion)
doi:10.21236/ada461237 · journal-paper full read; companion report itself inaccessible (see Access)

### Mechanism
EMA (Emotion and Adaptation) models emotion as a two-stage control system over a plan-based *causal interpretation* of the world: appraisal characterizes the person–environment relationship; coping repairs or maintains it. The 5-stage pipeline: (1) construct/maintain the causal interpretation (beliefs, desires, plans, intentions over past/present/future); (2) generate multiple *appraisal frames* — one per facilitation/inhibition relation per perspective (self + imagined others); (3) map each frame to an emotion instance via Elliott/OCC rules; (4) aggregate into current emotional state + slow mood; (5) adopt coping strategies, which act as the *inverse of appraisal* — they alter the causal-interpretation features (beliefs, goals, plans, intentions) that were the appraisal's antecedents, then re-appraisal follows. Key loop: threat → coping shifts blame → anger (re-appraisal). Focus: cognitive operators bring appraisal frames into focus (spreading-activation style); mood biases which in-focus instance wins.

### Formal core — the portable derivation rules (verbatim, from Gratch & Marsella 2004)
**Appraisal variables (Table 1):** Relevance (does the event require attention); Desirability (facilitates/thwarts wants); Causal attribution — Agency (who caused it), Blame/Credit (do they deserve it); Likelihood; Unexpectedness; Urgency; Ego involvement; Coping potential — Controllability, Changeability, Power, Adaptability.
**Derivation rules (Sec 4.2):**
- Relevance: significance ≡ utility; frame built only if |utility| of facilitated/inhibited state > threshold (1.0 in their apps).
- Desirability: desirable iff it *facilitates* a positively-valued state or *inhibits* a negatively-valued state (inhibition ≡ causal-threat relation in the plan); magnitude of utility = intensity variable.
- Likelihood: ≡ event probability; a threshold separates "certain" (Joy/Distress) from "uncertain" (Hope/Fear).
- Causal attribution: assigned to the *executing agent*; credit/blame weight = Desirability × Likelihood of the outcome.
- Controllability: max over plan actions that could re-establish a threatened goal (planning "white knight") of their likelihood — computed by finding actions in the causal interpretation whose effects impinge on the appraised event.
- Changeability: likelihood the event changes *without* the agent's intervention (uncertain effects, others' intervening acts).
- Perspective: frames built from self's AND imagined others' preference structures — the social-emotion machinery (guilt = self blamed for outcome another finds undesirable).
**Emotion mapping (Table 3):** Desir(p)>0,Lik(p)<1 → Hope; >0,Lik=1 → Joy; <0,Lik<1 → Fear; <0,Lik=1 → Distress; <0 + causal agent blameworthy → Anger; other q's Desir(q)<0 + self p blamed → Guilt. Intensity = |Desirability(p) × Likelihood(p)|.
**Mood:** per-type sum of intensities through a sigmoid; mood added to in-focus instance intensities (mood-biased focus).
**Coping selection tie-breaks:** propose strategies in parallel, adopt sequentially; prefer problem-directed (act, plan, seek info) when Controllability high; procrastination when Changeability high; emotion-focused (denial, disengagement, acceptance, reinterpretation) when both low. Strategy inventory: Action, Planning, Seek instrumental support, Procrastination, Positive reinterpretation, Acceptance, Denial, Mental disengagement, Shift blame, Seek/suppress information, Resignation.

### Firmware portability
**DESIGN** (with DIRECT fragments) — Full EMA needs a plan representation; the creature has none. But the appraisal *frame pattern* is directly portable to the 26-rule gate: each rule condition is an appraisal frame over a *drive/body-state relation* instead of a plan relation. Controllability ≈ "does some action in the repertoire undo this drive deviation?" (white-knight test on the action set — cheap and concrete); Changeability ≈ "will this deviation decay on its own?" (decay rate of the subsystem); Desirability ≈ sign × magnitude of drive deviation × drive weight; Likelihood ≈ predictor confidence on the drive's trajectory. The emotion-mapping table becomes a rule-priority/emotion-label table: Hope/Fear/Joy/Distress from (signed deviation × confidence), Anger/Guilt from attribution (partner-caused vs self-caused deviation) — giving the creature's face/expressive layer a principled vocabulary. The coping tie-break is directly a gate policy: **high controllability → act/plan rules; high changeability → wait rules; low both → internal regulation (disengage/reinterpret) rules**.

### Design answers
- **Q6 (body→action beyond gate veto)** — the sharpest answer: EMA shows how rule conditions can be *generated* rather than hand-authored. Replace "plan causal links" with "drive↔action causal links": for each drive deviation, derive controllability (is there an action that reverses it), changeability (does it self-decay), attribution (who/what caused it). These derived variables select among coping families, which map onto gate rule families. This is the missing generation layer for the gate.
- **Q2 (valence)**: EMA's Desirability = signed utility impact = a *valence* quantity, and it does double duty (categorical separation + intensity). Supports formalizing valence as signed goal-impact magnitude; but note EMA's desirability is cognitive (plan-relative), while our firmware valence should combine it with the homeostatic variant (Papers 2–3).
- **Q8 (social)**: perspective-taking frames are EMA's social mechanism — appraise the *other's* desirability structure from their perspective to generate guilt/shame/anticipatory guilt. Pairs with Paper 3's coupling channel: coupling handles felt other-distress; appraisal handles attributed social evaluation.
- **Q3 (boredom)**: not directly; EMA's "mental disengagement" coping (use other activities to take mind off the problem) is the closest precedent — an understimulation response could reuse the disengagement rule family.

### Access
Companion technical report (ADA461237) inaccessible — doi.org returned HTTP 500 after retries; per task constraints not retried via alternate endpoints. **Partial-plus**: the published journal paper (Gratch & Marsella 2004, Cognitive Systems Research 5(4):269–306) read in full via the authors' hosted PDF (3194 lines incl. Tables 1–3, all derivation rules, focus/mood, and the 5-stage coping process); the journal paper cites the companion (Gratch & Marsella 2004b) for the detailed rule listing. Derivation rules above are verbatim from the journal paper.

---

## Paper 5 — Allostasis: A model of predictive regulation (Sterling)
doi:10.1016/j.physbeh.2011.06.004 · Physiology & Behavior 106(1):5–15 (2012) · full chapter text read; the journal paper itself paywalled (see Access)

### Mechanism
Allostasis ("stability through change") replaces homeostasis ("stability through constancy") as the core regulatory model. The claim: the goal of regulation is not to clamp parameters at fixed setpoints via error-correcting feedback — feedback is ubiquitous but too inefficient to be primary. Instead, the brain *predicts* what levels will be needed and overrides local feedback to meet anticipated demand. Predictive regulation buys four advantages (2012 abstract): (i) errors are reduced in magnitude and frequency; (ii) response capacities of different components are matched (no bottlenecks, smaller safety factors); (iii) resources are shared between systems to minimize reserve capacities; (iv) errors are remembered and used to reduce future errors. A central organ (the brain) continuously integrates sensed variables with prior knowledge, sets priorities, enforces flexible trade-offs — "from each organ according to its ability, to each organ according to its need" — and governs *behavior* as a regulatory effector: the animal moves to a warmer place *before* it cools. Behavior runs on continuously updated "shopping lists" of specific appetites (warmth, food, salt, water), funneled into a common pathway with a "stick" that drives the organism toward filling the need (broadly, anxiety) and a "carrot" that relaxes it when satisfied (broadly, pleasure).

### Formal core — the six principles verbatim (Sterling, Principles of allostasis)
1. Organisms are designed for efficiency — systems sized to most-likely loads plus a modest safety factor; symmorphosis (capacities mutually matched).
2. Efficiency requires reciprocal trade-offs — resources are *loaned* between organs (e.g. at peak effort ~10% of muscle blood flow is borrowed from renal/splanchnic/skin); requires central control to monitor, prioritize, and schedule repayment.
3. Efficiency requires predicting what will be needed — adjust parameters to anticipated demand *before* the error occurs (insulin released at sight/smell of food, before glucose arrives).
4. Prediction requires each sensor to adapt its sensitivity to the expected range of input — sigmoid I/O curves rematch their steep region to the most-likely loads; two prediction levels: (a) most likely state next moment (current state + rate of change), (b) most likely time course (persistence).
5. Prediction requires each effector to adapt its output to the expected range of demand — receptors downregulate under sustained ligand ("the system learns that blood glucose is supposed to be high"); sustained demand teaches effectors to *expect* the new level.
6. Predictive regulation depends on behavior whose neural mechanisms also adapt — prefrontal integration of cascaded sensory + limbic inputs; emotion focuses/fixes intent ("instability of intent" without it); affect expression is itself socially modulated for resource-sharing exchanges.

### Firmware portability
**DESIGN** (shapes the architecture; fragments DIRECT) — Allostasis is the *philosophy* our somatic subsystem already follows (anticipate need, don't correct error). Portable fragments: (a) **prediction-level rule**: each drive forecasts next-state as state + rate-of-change, and adjusts *before* deviation — this is exactly how our somatic update should run (feedforward, not feedback); (b) **sensor adaptation**: each subsystem's sensitivity curve should recenter on the expected input range (running mean/range tracking — cheap); (c) **reciprocal trade-off scheduler**: the "budget" idea — when one drive borrows regulation capacity, another lends, with scheduled repayment; (d) **anxiety/pleasure as common pathway**: stick (growing need → anxiety-like arousal signal) + carrot (need satisfied → pleasure/relaxation) — a two-channel readout the face/display layer can render directly.

### Design answers
- **Q4 (moving setpoints)** — the direct answer: setpoints *should* move; the "defended level" of a regulated variable changes to optimally cope with demand (the chapter's term for shifting setpoints is rheostasis, Mrosovsky). Principle 5 gives the mechanism: sustained demand → effectors adapt → the system *learns that the new level is normal*. Port: implement setpoint drift as a slow EMA toward sustained demand, with faster drift when prediction error stays one-signed; keep a "repayment schedule" so borrowed capacity returns. Also: growth-stage setpoint schedules (Q4 across development) can be defined as staged changes in the defended levels — e.g. newborn defends narrow ranges, adolescent defends wider ones — which is allostasis-as-development.
- **Q2 (valence)**: the stick/carrot gives a two-part answer — *anxiety-like signal = growing need on the shopping list* (anticipatory, pre-error), *pleasure = need satisfaction*. Formal valence candidates: (1) negative valence ∝ predicted unmet need (the stick), positive valence ∝ need-resolution rate (the carrot); combined with Paper 3's valence-from-(distress, PE), this gives valence = f(predicted need, its rate of change, its surprise). Note Sterling's affect is *behavioral-drive affect*, not cognitive appraisal — complements EMA's Desirability.
- **Q1 (drive aggregation)**: Principle 2 is the answer — drives aggregate through a *central priority scheduler with reciprocal loans*, not additive summation: at any moment one drive borrows regulation capacity from others under a repayment schedule; conflicts produce "unpleasant sensations" (the gate's VETO analogue). Port: drives don't sum; they *bid for a shared regulatory budget*, with the scheduler enforcing priorities.
- **Q7 (sleep)**: Principle (iv) — "errors are remembered and used to reduce future errors." The persistent store is the *prediction machinery itself* (adapted sensor curves, downregulated receptors, learned shopping lists). Port: what survives sleep is not raw event logs but *updated parameters*: recentered sensitivity curves, drifted setpoints, learned demand forecasts. The dream pass should rewrite these parameters (sensor recentering, setpoint drift, forecast-model updates), not replay episodes.
- **Q3 (boredom)**: Sterling's "shopping lists" framing suggests boredom = an *unfilled appetite with no specific object* — a growing generic need (stimulation) on the list with no available replenishment path. Combined with the anxiety stick: understimulation registers as the stick firing with no satisfiable target.

### Access
The 2012 journal paper (Phys&Beh 106(1):5–15) is paywalled — abstract recovered verbatim via a secondary abstract listing. **Full chapter read**: Sterling's "Principles of allostasis" chapter (Cambridge UP 2004, hosted on his UPenn site) read in full — it is the primary statement of the same six principles, read at the source with all principle formulations verified verbatim. The six principles above are quoted faithfully from it.

---

## Cross-paper notes for the firmware (Part 4 synthesis)
- **Social channel**: two complementary mechanisms — Paper 3's *coupling* (d^cpl = d^self + λ·d^other, routed into the homeostat before planning; DIRECT) and Paper 1's *partner-local precision* (one confidence scalar per partner modulating action-selection temperature; DIRECT). Use both: coupling for felt other-regard, precision for per-partner commitment.
- **Gate rule generation (Q6)**: Paper 4's appraisal derivation rules port as drive-level frames — controllability (white-knight test on action set), changeability (self-decay rate), attribution (who caused the deviation), desirability (signed drive impact). Generate gate conditions from these instead of hand-authoring all 26.
- **Valence (Q2)**: three converging definitions — rate of homeostatic need/prediction error (Paper 3), signed goal impact (Paper 4 desirability), stick/carrot of predicted need (Paper 5). Formalize as valence = f(predicted need, need-resolution rate, homeostatic PE).
- **Setpoints (Q4)**: move by design (Paper 5, principle 5); drift toward sustained demand with repayment schedules; staged per growth stage.
- **Sleep persistence (Q7)**: persist *parameters*, not episodes — Dirichlet counts (Paper 2), recentered sensitivity curves + drifted setpoints (Paper 5); dream pass reweights these.
- **Drive aggregation (Q1)**: not plain additive — precision-weighted combination (Paper 1, Eq.4) for social/policy options, central budget scheduler with reciprocal loans (Paper 5) for physiological drives.
