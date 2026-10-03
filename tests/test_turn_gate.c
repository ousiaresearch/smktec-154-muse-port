/* Host test for muse_turn_gate.c (the PTT middleware).
 *
 * Stubs: tests/stubs/{esp_log.h,esp_err.h,muse_state.h,stub_fns.c}.
 * The brain/gate/diary-headers are real; the IDF is stubbed.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "muse_turn_gate.h"
#include "muse_brain.h"
#include "muse_gate.h"
#include "muse_state.h"   /* stub: gives stub_caption */

/* from stub_fns.c */
const char *stub_diary_last_line(void);
int stub_verdict_seen(void);
const char *stub_caption_seen(void);

static int failures = 0;
#define CHECK(cond, name) do { \
    if (!(cond)) { printf("FAIL %s\n", name); failures++; } \
} while (0)

static void cap_log(const char *line) { (void)line; }

/* Drive the brain into a veto: dead battery + terrible valence
 * -> appraisal gut STOP -> gate veto_gut_stop. */
static void make_veto_state(muse_brain_state_t *b)
{
    muse_brain_feed_battery(b, 3.05f, false, 1000);  /* energy ~0.04 */
    muse_brain_feed_valence(b, -1.0f, 2000);          /* event pulse */
    muse_brain_tick(b, 3000, false);                  /* derive valence */
}

int main(void)
{
    /* 1. Not attached: fail open, never strand the user. */
    CHECK(muse_turn_gate_veto() == false, "gate: unattached never vetoes");

    /* 2. Healthy brain: proceed. */
    muse_brain_state_t b;
    muse_identity_t id;
    memset(&id, 0, sizeof(id));
    id.growth_stage = 1;   /* toddler */
    muse_brain_init(&b, cap_log);
    muse_brain_feed_battery(&b, 4.0f, false, 1000);
    muse_turn_gate_attach(&b, &id);
    CHECK(muse_turn_gate_veto() == false, "gate: healthy turn proceeds");
    CHECK(muse_turn_gate_verdict() == MUSE_GATE_PROCEED,
          "gate: verdict proceed recorded");
    CHECK(stub_verdict_seen() == 0, "gate: face tint cleared on proceed");

    /* 3. Veto condition, toddler: the press is swallowed. */
    make_veto_state(&b);
    CHECK(muse_turn_gate_veto() == true, "gate: veto swallows the press");
    CHECK(muse_turn_gate_verdict() == MUSE_GATE_VETO,
          "gate: verdict veto recorded");
    CHECK(strcmp(muse_turn_gate_reason(), "veto_gut_stop") == 0,
          "gate: reason is veto_gut_stop");
    CHECK(stub_verdict_seen() == 2, "gate: face tint set to veto");
    CHECK(strstr(stub_caption_seen(), "NOT NOW") != NULL,
          "gate: caption says NOT NOW");
    CHECK(strstr(stub_diary_last_line(), "veto:") != NULL,
          "gate: veto logged to diary");

    /* 4. Veto condition, newborn: degraded to caution, press goes through. */
    id.growth_stage = 0;
    CHECK(muse_turn_gate_veto() == false,
          "gate: newborn veto degrades (press allowed)");
    CHECK(muse_turn_gate_verdict() == MUSE_GATE_CAUTION,
          "gate: newborn verdict is caution");
    CHECK(stub_verdict_seen() == 1, "gate: face tint set to caution");

    /* 5. Context hook: the snapshot rides along. */
    char ctx[1152];
    size_t n = muse_turn_context(ctx, sizeof(ctx));
    CHECK(n > 0 && n < sizeof(ctx), "gate: context hook returns snapshot");
    ctx[n] = '\0';
    CHECK(strstr(ctx, "\"self\"") != NULL, "gate: snapshot has identity");

    /* 6. Context hook with no brain: silent. */
    muse_turn_gate_attach(NULL, NULL);
    CHECK(muse_turn_context(ctx, sizeof(ctx)) == 0,
          "gate: unattached context hook is silent");

    if (failures == 0)
        printf("all turn-gate tests passed\n");
    else
        printf("%d FAILURES\n", failures);
    return failures != 0;
}
