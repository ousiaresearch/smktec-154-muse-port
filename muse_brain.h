/*
 * muse_brain.h — the on-device nervous system (v1).
 *
 * One struct per biomimetic subsystem (Phase 2 v1 set: scn, somatic,
 * fatigue, lc, dmn, hippocampus, decisions), folded into a single state
 * snapshot. Feeders push sensor readings in; tick() decays and ages them;
 * gate_inputs() assembles what muse_gate_evaluate() needs; consolidate()
 * runs the dream pass on SLEEPY entry.
 *
 * Staleness rule (from the kit): a quiet subsystem reads stale, never
 * lies. Each subsystem carries updated_ms; tick() flags stale ones, and
 * gate assembly substitutes neutral defaults for stale readings.
 *
 * The diary sink is a callback: the brain emits plain-text lines, the
 * board file appends them to SD. The brain never touches the filesystem.
 *
 * Pure C, no ESP-IDF dependency: unit-testable on the host.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "muse_gate.h"
#include "muse_identity.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Staleness budgets (ms). From the kit: SCN tolerates 2h; fast signals seconds. */
#define MUSE_SCN_STALE_MS        (2u * 3600u * 1000u)
#define MUSE_SOMATIC_STALE_MS    (60u * 1000u)
#define MUSE_FATIGUE_STALE_MS    (5u * 60u * 1000u)
#define MUSE_LC_STALE_MS         (30u * 1000u)
#define MUSE_DMN_STALE_MS        (5u * 60u * 1000u)
#define MUSE_HIPPOCAMPUS_STALE_MS (24u * 3600u * 1000u)

/* ALMA timescales (research intake): affect runs on three clocks.
 * Fast (seconds): arousal, tension, alertness, surprise. Medium
 * (minutes–hours): mood. Slow (days): traits, growth stage. */
#define MUSE_DECAY_FAST   0.90f   /* per 2s tick */
#define MUSE_DECAY_MEDIUM 0.99f   /* per 2s tick */
#define MUSE_DECAY_SLOW   0.995f  /* per 2s tick */

/* Homeostatic setpoints (research intake: Keramati & Gutkin 2014).
 * Drive = distance from setpoint; the creature wants things. */
#define MUSE_SETPOINT_ENERGY  0.80f
#define MUSE_SETPOINT_FATIGUE 0.15f
#define MUSE_SETPOINT_TENSION 0.20f

/* Homeostatic drive shape (Yoshida et al., Keramati & Gutkin):
 * d = (Σ|dev_i|^m)^(1/n). m=n=2 → Euclidean (firmware default).
 * m>n>1 gives deprivation potentiation, cross-need competition, and
 * concave (risk-averse) reward — free from the geometry. */
#define MUSE_DRIVE_M 2.0f
#define MUSE_DRIVE_N 2.0f

/* L2 valence (SYSTEMS.md): the emotion quadrant from Joffily &
 * Coricelli — sign(velocity of improvement) × sign(acceleration).
 * Relief/disappointment are sign flips of the derivative itself. */
typedef enum {
    MUSE_EMO_CALM = 0,       /* |v| below deadband */
    MUSE_EMO_HOPE,           /* improving, accelerating */
    MUSE_EMO_HAPPINESS,      /* improving, decelerating */
    MUSE_EMO_FEAR,           /* worsening, accelerating */
    MUSE_EMO_UNHAPPINESS,    /* worsening, decelerating */
    MUSE_EMO_RELIEF,         /* v flipped − to + */
    MUSE_EMO_DISAPPOINTMENT  /* v flipped + to − */
} muse_emotion_t;

const char *muse_emotion_name(muse_emotion_t e);

/* Valence derivation gain: maps per-tick drive change to [−1,1].
 * Drive moves ~0.01–0.1 per 2s tick; ×10 makes a 0.1 improvement
 * read as full positive valence. */
#define MUSE_VALENCE_GAIN 10.0f
/* Below this |v|, the creature is calm — no emotion named. */
#define MUSE_EMO_DEADBAND 0.05f
/* Joffily lr_eff = lr · exp(−k·v + ω): sensitivity of learning to
 * valence. k=1 gives e^1 ≈ 2.7× faster learning at v=−1. */
#define MUSE_LR_VALENCE_K 1.0f
/* Doya baselines: the modulators relax toward these. */
#define MUSE_HT_GAMMA_BASE 0.85f
#define MUSE_ACH_ALPHA_BASE 0.7f

typedef struct {
    float phase;          /* 0..1 circadian phase */
    bool quiet_hours;     /* 23:00–05:30 local, or entrained */
    uint32_t updated_ms;
    bool stale;
} muse_scn_t;

typedef struct {
    float energy;         /* 0..1 — battery-derived */
    float tension;        /* 0..1 — IMU agitation-derived */
    float arousal;        /* 0..1, fast layer */
    float valence;        /* -1..1, fast layer: DERIVED as −Δdrive/Δt
                           * plus a transient event pulse (see feed_valence).
                           * Never assigned directly. */
    float mood;           /* -1..1, medium layer (ω): slow EMA of derived
                           * valence — signed model-fitness. Persists. */
    uint32_t updated_ms;
    bool stale;
} muse_somatic_t;

typedef struct {
    float level;          /* 0..1 */
    float load;           /* 0..1 cognitive load estimate */
    bool recovery_needed; /* level > 0.85 */
    uint32_t updated_ms;
    bool stale;
} muse_fatigue_t;

typedef struct {
    float alertness;      /* 0..1 — feeds gate arousal */
    uint32_t updated_ms;
    bool stale;
} muse_lc_t;

typedef struct {
    uint32_t rest_ms;     /* stillness accumulator */
    uint32_t wanders;     /* mind-wandering episodes */
    uint32_t updated_ms;
    bool stale;
} muse_dmn_t;

typedef struct {
    uint32_t entries;     /* diary entries written */
    uint32_t places;      /* distinct WiFi BSSIDs seen */
    uint32_t updated_ms;
    bool stale;
} muse_hippocampus_t;

typedef struct {
    muse_gate_result_t last;
    uint32_t updated_ms;
} muse_decisions_t;

/* L4 appraisal + continuous vote (Smith & Read; Cathexis; EMA;
 * SYSTEMS.md). Drives stay modular; they combine late, at the action:
 * score(a) = Σ_d κ_d · r_d(a) — multiplicative within a drive
 * (κ scales only its own row), additive across, argmax on top.
 * The 26 rules remain the hard constraint layer above the vote. */
#define MUSE_NDRIVES 4

typedef enum {
    MUSE_DRIVE_HUNGER = 0,
    MUSE_DRIVE_FATIGUE = 1,
    MUSE_DRIVE_TENSION = 2,
    MUSE_DRIVE_INTEREST = 3   /* boredom as a drive (Cathexis) */
} muse_drive_id_t;

/* Appraisal frame (Gratch & Marsella, generated not hand-authored):
 * per-drive derivation of desirability / controllability /
 * changeability from the drive↔action causal links. */
typedef struct {
    float dev;             /* 0..1 deviation from setpoint */
    float desirability;    /* −1..1 signed value of the current state */
    float controllability; /* 0..1 can an action reverse it? (white-knight) */
    float changeability;   /* 0..1 will it self-decay? */
    float urgency;         /* dev × (1 − changeability) */
} muse_appraisal_t;

typedef enum {
    MUSE_ACT_PROCEED = 0,
    MUSE_ACT_CAUTION = 1,
    MUSE_ACT_VETO = 2
} muse_action_t;

typedef struct {
    muse_appraisal_t drives[MUSE_NDRIVES];
    muse_action_t vote;
    float vote_margin;
} muse_vote_t;

/* Diary sink: the board file wires this to SD append. Lines are short. */
typedef void (*muse_brain_log_fn)(const char *line);

typedef struct {
    muse_scn_t scn;
    muse_somatic_t somatic;
    muse_fatigue_t fatigue;
    muse_lc_t lc;
    muse_dmn_t dmn;
    muse_hippocampus_t hippocampus;
    muse_decisions_t decisions;
    /* Ephemeral (not subsystems, but the gate needs them): */
    float novelty;        /* 0..1 recent novelty rate */
    float vta;            /* 0..1 dopamine drive: prediction-error surprise */
    bool distracted;
    /* L2 valence derivation state (SYSTEMS.md): valence is computed in
     * tick() from drive change, never assigned. affect_pulse is the
     * transient event channel (pet +, error −) — the future social
     * coupling stub (λ·d^other): things that move the creature which
     * aren't in the drive model. Decays over minutes. */
    float drive_prev;     /* raw drive at the previous tick */
    bool drive_seeded;    /* false until the first fresh tick seeds it */
    float drive_reward;   /* HRRL reward r = d_prev − d: drive reduction
                           * this tick. Positive = the situation improved.
                           * Unclamped; v_fast is this × gain + pulse. */
    float affect_pulse;   /* -1..1 transient event input to valence */
    muse_emotion_t emotion;
    /* L6 boredom (Yu et al. HHVG, SYSTEMS.md): boredom = familiarity ×
     * (1 − info_gain). familiarity[] is the meta-model Q — one float
     * per context class (quiet × motion × recent-interaction = 16).
     * info_gain tracks recent prediction-error (vta). High boredom
     * lowers na_temp toward exploration (anti-darkroom). */
    float familiarity[16];
    float info_gain;      /* 0..1 recent prediction-error level */
    float boredom;        /* 0..1 */
    float na_temp;        /* Doya noradrenaline: inverse temperature β.
                           * 1 = exploit (sharp), 0 = explore (wide).
                           * Step 4 owns the interaction graph; the
                           * boredom coupling is set here. */
    float peak_boredom;
    float prev_boredom;
    /* L3 Doya modulators (SYSTEMS.md, corrected mapping): metaparameters,
     * not drives. Dopamine = TD error = fast valence (no new state —
     * v_fast IS δ). Serotonin = discount γ (horizon). Noradrenaline =
     * inverse temperature β (na_temp, from step 3). Acetylcholine =
     * global plasticity α; high = encode mode, low = retrieve mode.
     * da_* track δ's statistics for the Doya Fig. 9 interaction graph:
     * Var(δ) ⇒ γ down; sign-flips(δ) ⇒ α down (delta-bar-delta);
     * high γ ⇒ β,α down; |δ| (urgency) ⇒ β up. */
    float ht_gamma;       /* serotonin: 0..1 planning horizon */
    float ach_alpha;      /* acetylcholine: 0..1 global plasticity */
    float da_mean;        /* EMA of δ */
    float da_var;         /* EMA of Var(δ): world uncertainty */
    float da_flips;       /* EMA of δ sign-flip rate */
    float da_prev;        /* previous δ (flip detection) */
    muse_vote_t vote;     /* L4: appraisal frames + continuous vote */
    /* Curiosity predictor (research intake: Pathak et al. 2017, firmware
     * scale): EMA predictors per channel; surprise = |prediction-error|. */
    float pred_energy;
    float pred_tension;
    /* Learning progress (Oudeyer 2007): per-domain fast/slow EMAs of
     * success; progress = fast − slow. The slow EMA is persisted as
     * learn_base so progress survives deep sleep
     * (muse_brain_learning_save/restore). MUSE_LEARN_DOMAINS lives in
     * muse_identity.h, which persists the baseline. */
    float learn_fast[MUSE_LEARN_DOMAINS];
    float learn_slow[MUSE_LEARN_DOMAINS];
    /* Day counters for the dream pass: */
    uint32_t interactions;
    uint32_t veto_count;
    uint32_t caution_count;
    float peak_fatigue;
    uint32_t novel_count;
    char last_veto_reason[32];
    uint32_t last_interaction_ms;
    muse_brain_log_fn log;
} muse_brain_state_t;

void muse_brain_init(muse_brain_state_t *b, muse_brain_log_fn log);

/*
 * Advance time: decay arousal/tension toward rest, accrue fatigue with
 * load, recover faster when sleeping, age every subsystem into staleness.
 * Call at ~1 Hz from the board tick.
 */
void muse_brain_tick(muse_brain_state_t *b, uint32_t now_ms, bool sleeping);

/* Feeders — sensor readings in. */
void muse_brain_feed_battery(muse_brain_state_t *b, float voltage_v,
                             bool charging, uint32_t now_ms);
void muse_brain_feed_motion(muse_brain_state_t *b, float agitation_01,
                            uint32_t now_ms);
void muse_brain_feed_interaction(muse_brain_state_t *b, uint32_t now_ms);
void muse_brain_note_novelty(muse_brain_state_t *b, uint32_t now_ms);
void muse_brain_set_quiet_hours(muse_brain_state_t *b, bool quiet,
                                float phase, uint32_t now_ms);

/* Affect event (amy): pet +, error −. This does NOT set valence.
 * Valence is derived in tick() as −Δdrive/Δt. An event injects a
 * transient pulse into affect_pulse (the social-coupling channel):
 * a pet is a momentary lift, an error a momentary blow, both fading
 * over minutes. What the creature "feels" is drive change plus pulse. */
void muse_brain_feed_valence(muse_brain_state_t *b, float delta,
                             uint32_t now_ms);


/* Fill the appraisal frames from current state (pure derivation). */
void muse_brain_appraise(muse_brain_state_t *b);
/* The continuous vote: κ-weighted per-drive action values, argmax.
 * Runs appraise() first. Abstains (PROCEED, margin 0) when no drive
 * is active — then the rules decide alone. */
muse_action_t muse_brain_vote(muse_brain_state_t *b, float *margin_out);
/* Arbitration: the 26 rules are hard constraints (a rule VETO never
 * softens); the vote advises — it can urge caution, or soften an
 * advisory CAUTION when the creature is eager. The vote never vetoes
 * alone: hard vetoes need rule backing. */
muse_gate_verdict_t muse_resolve_verdict(muse_gate_verdict_t rules_v,
                                         muse_action_t vote, float margin);
/* Learning-rate law (Joffily & Coricelli eq. 4, Doya-scaled,
 * SYSTEMS.md steps 2+4): lr_eff = base · (α/α_base) · exp(−k·v + ω),
 * clamped so the exponential stays in [0.2×, 4×].
 * Negative valence (the model is failing) learns fast — the world may
 * have changed. Positive valence (the model is succeeding) learns slow
 * and consolidates. Mood ω is the persistent offset. The Doya
 * acetylcholine signal α is the global plasticity knob both rates
 * hang off. Steps 7–8 multiply in the precision-share and
 * windowed-stress gates. */
float muse_brain_lr_eff(const muse_brain_state_t *b, float base_lr);

/* Learning-progress feed (Oudeyer): success 0..1 per domain. */
void muse_brain_feed_learning(muse_brain_state_t *b, int domain,
                              float success01, uint32_t now_ms);

/* Persist/restore the slow learning EMAs across deep sleep. The board
 * calls restore after identity+brain init, and save runs inside
 * consolidate (persisted via muse_identity_save). Pure logic; the NVS
 * itself lives in the identity module. */
void muse_brain_learning_save(const muse_brain_state_t *b, muse_identity_t *id);
void muse_brain_learning_restore(muse_brain_state_t *b,
                                 const muse_identity_t *id);

/* Homeostatic drive (Keramati & Gutkin): 0..1 magnitude of need, plus the
 * dominant need name ("hunger"/"rest"/"calm"/"none"). Stale subsystems
 * don't contribute — the creature doesn't invent needs. */
float muse_brain_drive(const muse_brain_state_t *b, char *need_out,
                       size_t need_n);

/* Appraisal-based gut (research intake: EMA/Scherer sequential checking):
 * novelty → pleasantness → goal conduciveness → coping potential.
 * The cloud refines it later; on-device it just has to be honest. */
muse_gut_t muse_brain_suggest_gut(const muse_brain_state_t *b);

/* Fill gate inputs from brain state (neutral defaults for stale). */
void muse_brain_gate_inputs(const muse_brain_state_t *b, float adj_confidence,
                            muse_gut_t gut, muse_gate_inputs_t *out);

/* Growth (see GROWTH.md): care-day threshold and stage care-day marks. */
#define MUSE_CARE_DAY_INTERACTIONS 5u
#define MUSE_GROWTH_STAGES 4u

/*
 * The dream pass. Called on SLEEPY entry: evaluates the care-day, advances
 * the growth stage at its thresholds, folds the day's counters into a
 * diary entry via log(), then resets the counters. The muse dreams.
 * Caller persists the identity afterwards (muse_identity_save).
 */
void muse_brain_consolidate(muse_brain_state_t *b, muse_identity_t *id);

/*
 * Turn-injection snapshot (the firmware half of return-line): serialize
 * the brain into compact JSON for appending to the live turn — never to
 * the system prompt. ≤1100 chars so it can't crowd out the conversation.
 * Never fails and never blocks: stale reads are reported as "stale",
 * never invented. A context serializer must never be the reason a reply
 * doesn't happen.
 *
 * Returns bytes written (excluding NUL); 0 if out_n is too small.
 */
size_t muse_brain_snapshot(const muse_brain_state_t *b,
                           const muse_identity_t *id,
                           char *out, size_t out_n);

#ifdef __cplusplus
}
#endif
