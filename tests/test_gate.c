/* Host unit test for muse_gate.c — mirrors the Python rule semantics. */
#include "muse_gate.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static muse_gate_inputs_t neutral(void)
{
    muse_gate_inputs_t in;
    memset(&in, 0, sizeof(in));
    in.energy = 0.8f;
    in.arousal = 0.7f;
    in.adj_confidence = 0.8f;
    in.gut = MUSE_GUT_GO_AHEAD;
    return in;
}

static void check(const char *name, muse_gate_inputs_t in,
                  muse_gate_verdict_t want_v, const char *want_reason)
{
    muse_gate_result_t r = muse_gate_evaluate(&in);
    if (r.verdict != want_v || strcmp(r.reason, want_reason) != 0) {
        printf("FAIL %-28s got %s/%s want %s/%s\n", name,
               muse_gate_verdict_name(r.verdict), r.reason,
               muse_gate_verdict_name(want_v), want_reason);
        failures++;
    } else {
        printf("ok   %-28s %s/%s\n", name,
               muse_gate_verdict_name(r.verdict), r.reason);
    }
}

int main(void)
{
    muse_gate_inputs_t in;

    in = neutral();
    check("all neutral", in, MUSE_GATE_PROCEED, "proceed");

    in = neutral(); in.fatigue = 0.8f;
    check("veto_fatigue", in, MUSE_GATE_VETO, "veto_fatigue");

    in = neutral(); in.arousal = 0.2f;
    check("veto_low_arousal", in, MUSE_GATE_VETO, "veto_low_arousal");

    in = neutral(); in.gut = MUSE_GUT_STOP;
    check("veto_gut_stop", in, MUSE_GATE_VETO, "veto_gut_stop");

    in = neutral(); in.gut = MUSE_GUT_STOP; in.strong_habit = true;
    check("habit bypasses gut stop", in, MUSE_GATE_PROCEED, "proceed_habit_gut_stop");

    in = neutral(); in.gut = MUSE_GUT_PAUSE; in.strong_habit = true; in.energy = 0.1f;
    check("habit pause needs energy", in, MUSE_GATE_VETO, "veto_pause_energy");

    in = neutral(); in.gut = MUSE_GUT_PAUSE; in.quiet_hours = true;
    check("veto_quiet_gut", in, MUSE_GATE_VETO, "veto_quiet_gut");

    in = neutral(); in.quiet_hours = true;
    check("caution_quiet_hours", in, MUSE_GATE_CAUTION, "caution_quiet_hours");

    in = neutral(); in.fatigue = 0.6f;
    check("caution_fatigue", in, MUSE_GATE_CAUTION, "caution_fatigue");

    in = neutral(); in.novelty = 0.9f;
    check("proceed_novelty_extended", in, MUSE_GATE_PROCEED, "proceed_novelty_extended");

    in = neutral(); in.gut = MUSE_GUT_DOUBT; in.adj_confidence = 0.2f;
    check("veto_doubt_confidence", in, MUSE_GATE_VETO, "veto_doubt_confidence");

    in = neutral(); in.gut = MUSE_GUT_DOUBT; in.adj_confidence = 0.6f;
    check("caution_doubt", in, MUSE_GATE_CAUTION, "caution_doubt");

    in = neutral(); in.recovery_needed = true;
    check("veto_recovery first", in, MUSE_GATE_VETO, "veto_recovery");

    in = neutral(); in.drive = "recovery"; in.gut = MUSE_GUT_PAUSE;
    check("veto_recovery_state", in, MUSE_GATE_VETO, "veto_recovery_state");

    in = neutral(); in.distracted = true;
    check("caution_distracted", in, MUSE_GATE_CAUTION, "caution_distracted");

    in = neutral(); in.tension = 0.9f;
    check("caution_tension", in, MUSE_GATE_CAUTION, "caution_tension");

    printf(failures ? "\n%d FAILURES\n" : "\nall gate tests passed\n", failures);
    return failures != 0;
}
