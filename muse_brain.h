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

typedef struct {
    float phase;          /* 0..1 circadian phase */
    bool quiet_hours;     /* 23:00–05:30 local, or entrained */
    uint32_t updated_ms;
    bool stale;
} muse_scn_t;

typedef struct {
    float energy;         /* 0..1 — battery-derived */
    float tension;        /* 0..1 — IMU agitation-derived */
    float arousal;        /* 0..1 */
    float valence;        /* -1..1 */
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
    float vta;            /* 0..1 dopamine drive */
    bool distracted;
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

/* v1 heuristic gut feeling from somatic state; the cloud refines it later. */
muse_gut_t muse_brain_suggest_gut(const muse_brain_state_t *b);

/* Fill gate inputs from brain state (neutral defaults for stale). */
void muse_brain_gate_inputs(const muse_brain_state_t *b, float adj_confidence,
                            muse_gut_t gut, muse_gate_inputs_t *out);

/*
 * The dream pass. Called on SLEEPY entry: folds the day's counters into
 * a diary entry via log(), then resets the counters. The muse dreams.
 */
void muse_brain_consolidate(muse_brain_state_t *b);

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
size_t muse_brain_snapshot(const muse_brain_state_t *b, const char *name,
                           uint32_t generation, char *out, size_t out_n);

#ifdef __cplusplus
}
#endif
