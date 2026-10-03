/*
 * muse_turn_gate.c — the decision gate as turn middleware.
 *
 * See the header for the design. ESP-IDF draft — the muse_state and
 * muse_pixel calls are real SDK APIs; verify on the Mac build.
 */
#include "muse_turn_gate.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "muse_diary.h"
#include "muse_state.h"

/* Our renderer's verdict tint (avatar/muse_pixel.c). Declared here rather
 * than in a shared header: the pose struct stays SDK-compatible. */
extern void muse_pixel_set_verdict(int verdict);

static const char *TAG = "turn_gate";

static muse_brain_state_t *s_brain;
static muse_identity_t *s_id;
static muse_gate_verdict_t s_verdict = MUSE_GATE_PROCEED;
static char s_reason[48];

void muse_turn_gate_attach(muse_brain_state_t *brain, muse_identity_t *id)
{
    s_brain = brain;
    s_id = id;
}

muse_gate_verdict_t muse_turn_gate_verdict(void)
{
    return s_verdict;
}

const char *muse_turn_gate_reason(void)
{
    return s_reason[0] ? s_reason : "proceed";
}

/* The face shows the verdict: caption now, rim-light tint on the avatar.
 * The gate enum follows the Python exit codes (PROCEED=0, VETO=1,
 * CAUTION=2); the face takes 0 none/proceed, 1 caution, 2 veto. */
static void show_verdict(muse_gate_verdict_t v, const char *reason)
{
    s_verdict = v;
    strncpy(s_reason, reason ? reason : "", sizeof(s_reason) - 1);
    s_reason[sizeof(s_reason) - 1] = '\0';
    int face_v = (v == MUSE_GATE_VETO) ? 2 : (v == MUSE_GATE_CAUTION) ? 1 : 0;
    muse_pixel_set_verdict(face_v);
    if (v == MUSE_GATE_VETO) {
        muse_state_set_caption("NOT NOW");
    } else if (v == MUSE_GATE_CAUTION) {
        muse_state_set_caption("CAREFUL");
    }
}

bool muse_turn_gate_veto(void)
{
    if (!s_brain || !s_id) {
        return false;   /* not attached yet: fail open */
    }

    muse_gut_t gut = muse_brain_suggest_gut(s_brain);
    muse_gate_inputs_t in;
    /* No gesture at press time; 0.5 is the documented neutral. */
    muse_brain_gate_inputs(s_brain, 0.5f, gut, &in);
    muse_gate_result_t r = muse_gate_evaluate(&in);

    /* The newborn cannot say no — VETO degrades to CAUTION. */
    if (r.verdict == MUSE_GATE_VETO && s_id->growth_stage == 0) {
        ESP_LOGI(TAG, "newborn veto degraded: %s", r.reason);
        r.verdict = MUSE_GATE_CAUTION;
    }

    show_verdict(r.verdict, r.reason);

    if (r.verdict == MUSE_GATE_VETO) {
        char line[128];
        snprintf(line, sizeof(line), "veto: turn refused (%s)", r.reason);
        muse_diary_append(line);
        ESP_LOGI(TAG, "VETO %s", r.reason);
        return true;
    }
    if (r.verdict == MUSE_GATE_CAUTION) {
        char line[128];
        snprintf(line, sizeof(line), "caution: turn allowed (%s)", r.reason);
        muse_diary_append(line);
    }
    return false;
}

size_t muse_turn_context(char *out, size_t cap)
{
    if (!s_brain || !out || cap == 0) {
        return 0;
    }
    /* The snapshot is the continuity bridge: whoever serves this turn in
     * the cloud answers as Lapis-with-this-state. */
    size_t n = muse_brain_snapshot(s_brain, s_id, out, cap);
    if (n == 0 || n >= cap) {
        return 0;
    }
    return n;
}
