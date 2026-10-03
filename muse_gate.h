/*
 * muse_gate.h — the biomimetic decision gate, on device.
 *
 * Faithful C port of the rule semantics in biomimetic-brain's
 * scripts/decision-gate.py ("Gate Logic v4"): the same 26 rules, in the
 * same order, returning PROCEED / CAUTION / VETO plus the reason code that
 * fired. The Python reads JSON files; this takes a struct of readings so
 * it can run on the in-memory brain state with no filesystem, no heap,
 * no model calls.
 *
 * The gate is READ-ONLY: evaluating never mutates the inputs, and the
 * caller decides what quiet_hours means (scn phase or wall clock).
 * "Essential" actions bypass quiet hours by passing quiet_hours=false —
 * same as the Python's essential mode.
 *
 * Pure C, no ESP-IDF dependency: unit-testable on the host.
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MUSE_GATE_PROCEED = 0,
    MUSE_GATE_CAUTION = 2,  /* matches the Python exit codes */
    MUSE_GATE_VETO    = 1,
} muse_gate_verdict_t;

typedef enum {
    MUSE_GUT_GO_AHEAD = 0,
    MUSE_GUT_PAUSE,
    MUSE_GUT_STOP,
    MUSE_GUT_DOUBT,
    MUSE_GUT_WAIT,
} muse_gut_t;

/* All floats are 0..1 unless noted. */
typedef struct {
    float energy;
    float fatigue;
    float arousal;
    float tension;
    float novelty;
    float vta;               /* dopamine drive */
    float boredom;           /* 0..1 HHVG devaluation: familiar, uninformative */
    float explore_temp;      /* Doya β: 1 = exploit, 0 = explore */
    float adj_confidence;
    bool  recovery_needed;
    bool  distracted;
    bool  quiet_hours;
    bool  strong_habit;      /* basal-ganglia habit bypass */
    int   active_passions;   /* count > 0 fires */
    int   active_convictions;
    int   emergent_interests;
    muse_gut_t gut;
    const char *drive;       /* e.g. "recovery"; NULL if none */
} muse_gate_inputs_t;

typedef struct {
    muse_gate_verdict_t verdict;
    const char *reason;      /* static string, e.g. "veto_fatigue" */
} muse_gate_result_t;

/* Evaluate the rules in spec order; first match wins. */
muse_gate_result_t muse_gate_evaluate(const muse_gate_inputs_t *in);

/* "PROCEED" / "CAUTION" / "VETO" — for logs, the face, the diary. */
const char *muse_gate_verdict_name(muse_gate_verdict_t v);

#ifdef __cplusplus
}
#endif
