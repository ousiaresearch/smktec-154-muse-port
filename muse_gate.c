/*
 * muse_gate.c — decision gate rule evaluation.
 *
 * Direct transliteration of RULES in biomimetic-brain's
 * scripts/decision-gate.py. Rule order is load-bearing: the habit bypass
 * runs before the gut-based vetoes, and PROCEED EXTENDED runs right after
 * VETO, before CAUTION. Do not reorder without consulting docs/DECISION_GATE.md.
 */
#include "muse_gate.h"

#include <string.h>

muse_gate_result_t muse_gate_evaluate(const muse_gate_inputs_t *in)
{
    const bool gut_pause = in->gut == MUSE_GUT_PAUSE;
    const bool gut_stop  = in->gut == MUSE_GUT_STOP;
    const bool gut_doubt = in->gut == MUSE_GUT_DOUBT;
    const bool gut_wait  = in->gut == MUSE_GUT_WAIT;
    const bool gut_go    = in->gut == MUSE_GUT_GO_AHEAD;
    const bool drive_recovery = in->drive && strcmp(in->drive, "recovery") == 0;

    /* ── VETO — the non-gut vetoes ── */
    if (in->recovery_needed)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_recovery" };
    if (in->fatigue > 0.70f)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_fatigue" };
    if (in->arousal < 0.30f)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_low_arousal" };
    if (drive_recovery && (gut_pause || gut_stop))
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_recovery_state" };

    /* ── BASAL GANGLIA HABIT BYPASS (before the gut-based vetoes) ── */
    if (in->strong_habit && gut_stop)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_habit_gut_stop" };
    if (in->strong_habit && gut_pause && in->energy >= 0.20f)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_habit_gut_pause" };
    if (in->strong_habit && gut_doubt && in->adj_confidence >= 0.30f)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_habit_gut_doubt" };

    /* ── VETO — the gut-based vetoes ── */
    if (gut_stop)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_gut_stop" };
    if (in->distracted && gut_pause)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_distracted_pause" };
    if (in->distracted && gut_doubt && in->adj_confidence < 0.50f)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_distracted_doubt" };
    if (gut_pause && in->energy < 0.35f)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_pause_energy" };
    if (gut_doubt && in->adj_confidence < 0.40f)
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_doubt_confidence" };
    if (in->quiet_hours && (gut_pause || gut_stop))
        return (muse_gate_result_t){ MUSE_GATE_VETO, "veto_quiet_gut" };

    /* ── PROCEED EXTENDED — right after VETO, before CAUTION ── */
    if (in->active_passions > 0)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_passion" };
    if (in->active_convictions > 0)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_conviction" };
    if (in->emergent_interests > 0)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_curiosity_extended" };
    if (in->novelty > 0.7f && in->vta > 0.7f)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_extended_surge" };
    if (in->novelty > 0.7f)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_novelty_extended" };
    if (in->vta > 0.7f)
        return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed_vta_extended" };

    /* ── CAUTION ── */
    if (in->distracted && gut_go)
        return (muse_gate_result_t){ MUSE_GATE_CAUTION, "caution_distracted" };
    if (gut_pause || gut_wait)
        return (muse_gate_result_t){ MUSE_GATE_CAUTION, "caution_gut" };
    if (gut_doubt && in->adj_confidence >= 0.40f)
        return (muse_gate_result_t){ MUSE_GATE_CAUTION, "caution_doubt" };
    if (in->fatigue > 0.50f)
        return (muse_gate_result_t){ MUSE_GATE_CAUTION, "caution_fatigue" };
    if (in->arousal < 0.50f)
        return (muse_gate_result_t){ MUSE_GATE_CAUTION, "caution_low_arousal" };
    if (in->tension > 0.70f)
        return (muse_gate_result_t){ MUSE_GATE_CAUTION, "caution_tension" };
    if (in->quiet_hours)
        return (muse_gate_result_t){ MUSE_GATE_CAUTION, "caution_quiet_hours" };

    return (muse_gate_result_t){ MUSE_GATE_PROCEED, "proceed" };
}

const char *muse_gate_verdict_name(muse_gate_verdict_t v)
{
    switch (v) {
    case MUSE_GATE_PROCEED: return "PROCEED";
    case MUSE_GATE_CAUTION: return "CAUTION";
    case MUSE_GATE_VETO:    return "VETO";
    }
    return "?";
}
