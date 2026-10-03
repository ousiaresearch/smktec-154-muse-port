/* Host unit test for muse_brain.c (+ muse_gate.c). */
#include "muse_brain.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static char last_log[512];

static void cap_log(const char *line)
{
    size_t cur = strlen(last_log);
    size_t room = sizeof(last_log) - cur - 1;
    if (cur > 0 && room > 1) {
        last_log[cur] = '|';
        last_log[cur + 1] = '\0';
        cur++;
        room--;
    }
    strncat(last_log, line, room);
}

#define CHECK(cond, name) do { \
    if (!(cond)) { printf("FAIL %s\n", name); failures++; } \
    else { printf("ok   %s\n", name); } \
} while (0)

int main(void)
{
    muse_brain_state_t b;
    muse_brain_init(&b, cap_log);

    CHECK(b.scn.stale && b.somatic.stale, "starts honest: stale");

    muse_brain_feed_battery(&b, 4.1f, false, 1000);
    CHECK(!b.somatic.stale && b.somatic.energy > 0.9f, "battery 4.1V ~ full");
    muse_brain_feed_battery(&b, 3.1f, false, 2000);
    CHECK(b.somatic.energy < 0.1f, "battery 3.1V ~ empty");

    b.dmn.rest_ms = 5000;
    muse_brain_feed_motion(&b, 0.9f, 3000);
    CHECK(b.somatic.tension > 0.2f && b.lc.alertness > 0.5f &&
          b.dmn.rest_ms == 0, "motion -> tension/alert, rest reset");

    muse_brain_tick(&b, 3000 + MUSE_LC_STALE_MS + 1, false);
    CHECK(b.lc.stale, "lc goes stale after 30s quiet");

    b.fatigue.level = 0.8f; b.fatigue.load = 0.5f;
    for (int i = 0; i < 10; i++)
        muse_brain_tick(&b, 100000 + i * 1000, true);
    CHECK(b.fatigue.level < 0.8f, "sleep recovers fatigue");

    muse_brain_state_t b2;
    muse_brain_init(&b2, cap_log);
    muse_gate_inputs_t in;
    muse_brain_gate_inputs(&b2, 0.8f, MUSE_GUT_GO_AHEAD, &in);
    muse_gate_result_t r = muse_gate_evaluate(&in);
    CHECK(r.verdict == MUSE_GATE_PROCEED, "stale brain gates neutral PROCEED");

    muse_brain_feed_battery(&b2, 4.0f, false, 1000);
    b2.fatigue.level = 0.9f; b2.fatigue.updated_ms = 1000; b2.fatigue.stale = false;
    muse_brain_gate_inputs(&b2, 0.8f, MUSE_GUT_GO_AHEAD, &in);
    r = muse_gate_evaluate(&in);
    CHECK(r.verdict == MUSE_GATE_VETO, "exhausted brain vetoes");

    muse_brain_state_t b3;
    muse_brain_init(&b3, cap_log);
    muse_brain_feed_interaction(&b3, 1000);
    muse_brain_feed_interaction(&b3, 2000);
    muse_brain_note_novelty(&b3, 3000);
    b3.peak_fatigue = 0.81f;
    muse_identity_t id3;
    memset(&id3, 0, sizeof(id3));
    last_log[0] = '\0';
    muse_brain_consolidate(&b3, &id3);
    CHECK(strstr(last_log, "2 interactions") && strstr(last_log, "0.81") &&
          b3.interactions == 0 && b3.hippocampus.entries == 1 &&
          id3.care_days == 0,   /* 2 interactions < care-day threshold */
          "consolidate dreams + resets");
    printf("dream line: %s\n", last_log);

    muse_brain_state_t b4;
    muse_brain_init(&b4, cap_log);
    muse_brain_feed_battery(&b4, 3.2f, false, 1000);
    CHECK(muse_brain_suggest_gut(&b4) == MUSE_GUT_PAUSE, "low energy -> pause gut");

    /* Snapshot: compact JSON for turn injection, never fails. */
    muse_brain_state_t b5;
    muse_brain_init(&b5, cap_log);
    muse_identity_t id5;
    memset(&id5, 0, sizeof(id5));
    strncpy(id5.name, "Lapis", sizeof(id5.name) - 1);
    id5.generation = 3;
    muse_brain_feed_battery(&b5, 4.0f, false, 1000);
    muse_brain_feed_interaction(&b5, 2000);
    char snap[1100];
    size_t sn = muse_brain_snapshot(&b5, &id5, snap, sizeof(snap));
    CHECK(sn > 0 && sn < 1100 &&
          strstr(snap, "\"self\":\"Lapis\"") &&
          strstr(snap, "\"energy\":0.83") &&
          strstr(snap, "\"stage\":0"),
          "snapshot compact JSON");
    printf("snapshot: %s\n", snap);

    /* Fresh brain: stale reads report -1, never invented. */
    muse_brain_state_t b6;
    muse_brain_init(&b6, cap_log);
    sn = muse_brain_snapshot(&b6, &id5, snap, sizeof(snap));
    CHECK(sn > 0 && strstr(snap, "\"energy\":-1.00") &&
          strstr(snap, "\"stale\":[\"scn\""),
          "snapshot honest about staleness");

    /* Tiny buffer: returns 0, never a truncated lie. */
    char tiny[16];
    CHECK(muse_brain_snapshot(&b5, &id5, tiny, sizeof(tiny)) == 0,
          "snapshot refuses tiny buffer");

    /* Growth: care-days accumulate, stages advance, never regress. */
    muse_brain_state_t b7;
    muse_identity_t id7;
    memset(&id7, 0, sizeof(id7));
    last_log[0] = '\0';
    for (int day = 0; day < 3; day++) {
        muse_brain_init(&b7, cap_log);
        for (int i = 0; i < 6; i++)
            muse_brain_feed_interaction(&b7, (uint32_t)(day * 100000 + i * 1000));
        muse_brain_consolidate(&b7, &id7);
    }
    CHECK(id7.care_days == 3 && id7.growth_stage == 1 &&
          strstr(last_log, "stage 1"),
          "growth: toddler at 2 care-days, announced");
    /* A quiet day: no progress, no punishment. */
    muse_brain_init(&b7, cap_log);
    muse_brain_consolidate(&b7, &id7);
    CHECK(id7.care_days == 3 && id7.growth_stage == 1,
          "growth: quiet day changes nothing");

    printf(failures ? "\n%d FAILURES\n" : "\nall brain tests passed\n", failures);
    return failures != 0;
}
