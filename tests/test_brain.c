/* Host unit test for muse_brain.c (+ muse_gate.c). */
#include "muse_brain.h"

#include <math.h>
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

    /* Homeostatic drive: deviation from setpoint, named need. */
    muse_brain_state_t b8;
    muse_brain_init(&b8, cap_log);
    muse_brain_feed_battery(&b8, 3.2f, false, 1000);  /* energy ~0.17 */
    char need[16];
    float drive = muse_brain_drive(&b8, need, sizeof(need));
    CHECK(drive > 0.5f && strcmp(need, "hunger") == 0,
          "drive: low battery reads as hunger");
    /* Fresh brain: no invented needs. */
    muse_brain_state_t b8b;
    muse_brain_init(&b8b, cap_log);
    drive = muse_brain_drive(&b8b, need, sizeof(need));
    CHECK(drive == 0.0f && strcmp(need, "none") == 0,
          "drive: stale brain wants nothing");

    /* Appraisal gut (Scherer order): suddenness -> pleasantness ->
     * conduciveness -> coping. */
    muse_brain_state_t b9;
    muse_brain_init(&b9, cap_log);
    muse_brain_feed_battery(&b9, 3.2f, false, 1000);
    CHECK(muse_brain_suggest_gut(&b9) == MUSE_GUT_PAUSE,
          "appraisal: critical energy -> PAUSE (goal conduciveness)");
    muse_brain_feed_valence(&b9, -0.7f, 2000);
    muse_brain_tick(&b9, 2000, false);   /* derive valence from the pulse */
    CHECK(muse_brain_suggest_gut(&b9) == MUSE_GUT_DOUBT,
          "appraisal: bad valence -> DOUBT (pleasantness)");
    muse_brain_feed_valence(&b9, -0.3f, 3000);
    muse_brain_feed_battery(&b9, 3.05f, false, 4000);  /* energy ~0.04 */
    muse_brain_tick(&b9, 4000, false);
    CHECK(muse_brain_suggest_gut(&b9) == MUSE_GUT_STOP,
          "appraisal: deeply bad + no resources -> STOP");

    /* L2 valence (SYSTEMS.md): derived from drive change, not assigned.
     * Charge the creature (drive falls) -> positive valence, HOPE. */
    muse_brain_state_t b9b;
    muse_brain_init(&b9b, cap_log);
    muse_brain_feed_battery(&b9b, 3.2f, false, 1000);  /* drive high */
    muse_brain_tick(&b9b, 2000, false);                /* seed */
    CHECK(b9b.emotion == MUSE_EMO_CALM, "valence: seeded tick is calm");
    muse_brain_feed_battery(&b9b, 4.1f, true, 4000);   /* drive falls */
    muse_brain_tick(&b9b, 4000, false);
    CHECK(b9b.somatic.valence > 0.5f,
          "valence: falling drive feels good (-dD/dt)");
    CHECK(b9b.emotion == MUSE_EMO_HOPE,
          "valence: improving+accelerating reads as hope");
    /* Drain it again (drive rises) -> the flip names disappointment. */
    muse_brain_feed_battery(&b9b, 3.2f, false, 6000);
    muse_brain_tick(&b9b, 6000, false);
    CHECK(b9b.somatic.valence < -0.5f,
          "valence: rising drive feels bad");
    CHECK(b9b.emotion == MUSE_EMO_DISAPPOINTMENT,
          "valence: + to - flip reads as disappointment");
    CHECK(strcmp(muse_emotion_name(b9b.emotion), "disappointment") == 0,
          "valence: emotion names itself");

    /* ALMA medium layer: mood tracks valence slowly. */
    muse_brain_state_t b10;
    muse_brain_init(&b10, cap_log);
    muse_brain_feed_valence(&b10, 0.8f, 1000);
    float mood0 = b10.somatic.mood;
    for (int i = 0; i < 20; i++)
        muse_brain_tick(&b10, 2000 + i * 2000, false);
    CHECK(b10.somatic.mood > mood0 && b10.somatic.mood < 0.8f,
          "mood: slow EMA toward valence");
    CHECK(b10.somatic.valence < 0.8f,
          "valence: decays faster than mood");

    /* Curiosity as prediction error: surprise drives vta. */
    muse_brain_state_t b11;
    muse_brain_init(&b11, cap_log);
    muse_brain_feed_battery(&b11, 4.0f, false, 1000);
    muse_brain_feed_battery(&b11, 4.0f, false, 2000);
    float calm_vta = b11.vta;
    muse_brain_feed_battery(&b11, 3.3f, false, 3000);  /* sudden drop */
    CHECK(b11.vta > calm_vta + 0.2f,
          "curiosity: prediction error spikes vta");

    /* Learning progress (Oudeyer): improving success accrues mastery,
     * measured against the persisted baseline across days (deep sleep
     * wipes RAM between them). */
    muse_brain_state_t b12;
    muse_identity_t id12;
    memset(&id12, 0, sizeof(id12));
    id12.learn_base[0] = id12.learn_base[1] = 0.5f;
    for (int day = 0; day < 2; day++) {
        muse_brain_init(&b12, cap_log);            /* deep sleep wiped RAM */
        muse_brain_learning_restore(&b12, &id12);  /* baseline back */
        for (int i = 0; i < 8; i++) {
            muse_brain_feed_interaction(&b12, (uint32_t)(i * 1000));
            /* day 0: mediocre; day 1: clearly better, both domains */
            float s = day == 0 ? 0.4f : 0.85f;
            muse_brain_feed_learning(&b12, 0, s, (uint32_t)(i * 1000));
            muse_brain_feed_learning(&b12, 1, s, (uint32_t)(i * 1000));
        }
        muse_brain_consolidate(&b12, &id12);
    }
    CHECK(id12.mastery > 0, "learning: sustained progress earns mastery");
    CHECK(id12.learn_base[0] > 0.5f,
          "learning: capability baseline ratchets up");

    /* Joffily learning-rate law (SYSTEMS.md step 2):
     * lr_eff = lr · exp(−k·v + ω), clamped [0.2×, 4×]. */
    muse_brain_state_t b13;
    muse_brain_init(&b13, cap_log);
    b13.somatic.valence = -1.0f; b13.somatic.mood = 0.0f;
    CHECK(muse_brain_lr_eff(&b13, 0.1f) > 0.27f,
          "lr: negative valence learns ~2.7x faster");
    b13.somatic.valence = 1.0f;
    CHECK(muse_brain_lr_eff(&b13, 0.1f) < 0.04f,
          "lr: positive valence learns ~2.7x slower");
    b13.somatic.valence = 0.0f; b13.somatic.mood = 0.5f;
    CHECK(muse_brain_lr_eff(&b13, 0.1f) > 0.16f,
          "lr: good mood offsets toward faster learning");
    b13.somatic.valence = -1.0f; b13.somatic.mood = 1.0f;
    CHECK(fabsf(muse_brain_lr_eff(&b13, 0.1f) - 0.4f) < 1e-6f,
          "lr: multiplier clamps at 4x");
    b13.somatic.valence = 1.0f; b13.somatic.mood = -1.0f;
    CHECK(fabsf(muse_brain_lr_eff(&b13, 0.1f) - 0.02f) < 1e-6f,
          "lr: multiplier clamps at 0.2x");

    /* Behavioral: a creature that feels bad revises its model faster
     * than one that feels good, given the same evidence. */
    muse_brain_state_t b14a, b14b;
    muse_brain_init(&b14a, cap_log);
    muse_brain_init(&b14b, cap_log);
    muse_brain_feed_valence(&b14a, -1.0f, 1000);
    muse_brain_tick(&b14a, 1000, false);
    muse_brain_feed_valence(&b14b, 1.0f, 1000);
    muse_brain_tick(&b14b, 1000, false);
    for (int i = 0; i < 5; i++) {
        muse_brain_feed_learning(&b14a, 0, 1.0f, (uint32_t)(i * 1000));
        muse_brain_feed_learning(&b14b, 0, 1.0f, (uint32_t)(i * 1000));
    }
    CHECK(b14a.learn_fast[0] > b14b.learn_fast[0] + 0.1f,
          "lr: bad feeling revises the model faster");

    /* L6 boredom (Yu et al. HHVG, SYSTEMS.md step 3):
     * boredom = familiarity × (1 − info_gain). */
    muse_brain_state_t b15a, b15b;
    muse_brain_init(&b15a, cap_log);
    muse_brain_init(&b15b, cap_log);
    muse_brain_feed_battery(&b15a, 4.0f, false, 1000);
    muse_brain_feed_battery(&b15b, 4.0f, false, 1000);
    for (int i = 0; i < 600; i++) {
        /* b gets surprise every 5 ticks; a sits in the same quiet room. */
        if (i % 5 == 0)
            muse_brain_note_novelty(&b15b, (uint32_t)(1000 + i * 2000));
        muse_brain_tick(&b15a, (uint32_t)(2000 + i * 2000), false);
        muse_brain_tick(&b15b, (uint32_t)(2000 + i * 2000), false);
    }
    CHECK(b15a.boredom > 0.5f,
          "boredom: uneventful exposure breeds boredom");
    CHECK(b15b.boredom < b15a.boredom - 0.05f,
          "boredom: information gain suppresses it");
    CHECK(b15a.na_temp < 0.4f,
          "boredom: high boredom lowers exploration temp (Doya beta)");
    CHECK(b15a.dmn.wanders >= 1,
          "boredom: crossing into boredom is a wandering episode");
    CHECK(b15a.info_gain < 0.05f,
          "boredom: quiet room teaches nothing");
    char snap15[1152];
    CHECK(muse_brain_snapshot(&b15a, NULL, snap15, sizeof(snap15)) > 0 &&
          strstr(snap15, "\"boredom\"") != NULL,
          "boredom: snapshot reports it");

    /* L3 Doya modulators (SYSTEMS.md step 4, corrected mapping):
     * DA = TD error = fast valence; 5-HT = γ; NA = β; ACh = α. */
    muse_brain_state_t b16;
    muse_brain_init(&b16, cap_log);
    CHECK(fabsf(b16.ht_gamma - 0.85f) < 1e-6f &&
          fabsf(b16.ach_alpha - 0.7f) < 1e-6f &&
          fabsf(b16.na_temp - 0.5f) < 1e-6f,
          "doya: modulators start at baseline");
    /* An oscillating world: δ flips sign every tick. */
    for (int i = 0; i < 100; i++) {
        muse_brain_feed_valence(&b16, (i % 2 == 0) ? 2.0f : -2.0f,
                                (uint32_t)(1000 + i * 2000));
        muse_brain_tick(&b16, (uint32_t)(2000 + i * 2000), false);
    }
    CHECK(b16.da_var > 0.5f, "doya: oscillation registers as variance");
    CHECK(b16.ht_gamma < 0.8f,
          "doya: uncertain world shortens the horizon (5-HT down)");
    CHECK(b16.da_flips > 0.5f, "doya: flip rate tracked");
    CHECK(b16.ach_alpha < 0.65f,
          "doya: oscillating error lowers plasticity (delta-bar-delta)");
    /* α is the global plasticity knob on the learning law. */
    muse_brain_state_t b17;
    muse_brain_init(&b17, cap_log);
    b17.ach_alpha = 1.4f;
    {
        muse_brain_state_t b17b;
        muse_brain_init(&b17b, cap_log);   /* α at baseline */
        float r = muse_brain_lr_eff(&b17, 0.1f) /
                  muse_brain_lr_eff(&b17b, 0.1f);
        CHECK(fabsf(r - 2.0f) < 1e-4f,
              "doya: doubling ACh doubles the learning rate");
    }
    /* Urgency (|δ|) sharpens β toward exploitation. */
    muse_brain_state_t b18a, b18b;
    muse_brain_init(&b18a, cap_log);
    muse_brain_init(&b18b, cap_log);
    muse_brain_feed_valence(&b18a, 2.0f, 1000);
    for (int i = 0; i < 60; i++) {
        muse_brain_tick(&b18a, (uint32_t)(2000 + i * 2000), false);
        muse_brain_tick(&b18b, (uint32_t)(2000 + i * 2000), false);
    }
    CHECK(b18a.na_temp > b18b.na_temp + 0.05f,
          "doya: urgency sharpens the choice temperature");
    char snap18[1152];
    CHECK(muse_brain_snapshot(&b18a, NULL, snap18, sizeof(snap18)) > 0 &&
          strstr(snap18, "\"gamma\"") != NULL,
          "doya: snapshot reports the modulators");

    /* L1 Minkowski drive + HRRL reward (SYSTEMS.md step 5):
     * d = (Σ|dev|^m)^(1/n); r = d_prev − d. */
    muse_brain_state_t b19;
    muse_brain_init(&b19, cap_log);
    muse_brain_feed_battery(&b19, 3.2f, false, 1000);  /* drive 0.63 */
    muse_brain_tick(&b19, 2000, false);                /* seed */
    CHECK(fabsf(b19.drive_reward) < 1e-6f,
          "reward: seeded tick earns nothing");
    muse_brain_feed_battery(&b19, 4.1f, true, 4000);   /* drive falls */
    muse_brain_tick(&b19, 4000, false);
    CHECK(b19.drive_reward > 0.5f,
          "reward: drive reduction is positive reward");
    muse_brain_feed_battery(&b19, 3.2f, false, 6000);  /* drive rises */
    muse_brain_tick(&b19, 6000, false);
    CHECK(b19.drive_reward < -0.5f,
          "reward: drive increase is negative reward");
    CHECK(fabsf(b19.drive_prev - 0.6333f) < 0.01f,
          "drive: Minkowski matches Euclidean at m=n=2");
    char snap19[1152];
    CHECK(muse_brain_snapshot(&b19, NULL, snap19, sizeof(snap19)) > 0 &&
          strstr(snap19, "\"reward\"") != NULL,
          "reward: snapshot reports it");

    /* L4 appraisal frames (EMA, generated not hand-authored). */
    muse_brain_state_t b20;
    muse_brain_init(&b20, cap_log);
    b20.fatigue.level = 0.9f; b20.fatigue.stale = false;
    b20.fatigue.updated_ms = 1000;
    muse_brain_appraise(&b20);
    CHECK(fabsf(b20.vote.drives[MUSE_DRIVE_FATIGUE].dev - 0.75f) < 1e-6f,
          "appraisal: fatigue deviation measured");
    CHECK(fabsf(b20.vote.drives[MUSE_DRIVE_FATIGUE].urgency - 0.525f) < 1e-6f,
          "appraisal: urgency = dev x (1 - changeability)");
    CHECK(b20.vote.drives[MUSE_DRIVE_HUNGER].controllability == 0.0f,
          "appraisal: hunger not self-reversible (white-knight fails)");
    CHECK(b20.vote.drives[MUSE_DRIVE_TENSION].changeability == 0.9f,
          "appraisal: tension self-decays");
    /* Fresh brain: nothing appraised. */
    muse_brain_state_t b20b;
    muse_brain_init(&b20b, cap_log);
    muse_brain_appraise(&b20b);
    CHECK(b20b.vote.drives[MUSE_DRIVE_FATIGUE].urgency == 0.0f,
          "appraisal: stale subsystems appraise nothing");

    /* L4 continuous vote (Smith & Read): κ-weighted, argmax on top. */
    muse_brain_state_t b21;
    muse_brain_init(&b21, cap_log);
    float m21 = -1.0f;
    CHECK(muse_brain_vote(&b21, &m21) == MUSE_ACT_PROCEED && m21 == 0.0f,
          "vote: no active drive abstains (rules decide alone)");
    /* Boredom votes EAGER. */
    muse_brain_state_t b22;
    muse_brain_init(&b22, cap_log);
    b22.boredom = 0.9f;
    float m22 = 0.0f;
    CHECK(muse_brain_vote(&b22, &m22) == MUSE_ACT_PROCEED && m22 > 0.3f,
          "vote: boredom votes eager with a strong margin");
    /* Tension votes WARY. */
    muse_brain_state_t b23;
    muse_brain_init(&b23, cap_log);
    b23.somatic.stale = false;
    b23.somatic.updated_ms = 1000;
    b23.somatic.energy = 0.8f;    /* hunger 0 */
    b23.somatic.tension = 0.9f;   /* tense 0.7 */
    float m23 = 0.0f;
    CHECK(muse_brain_vote(&b23, &m23) == MUSE_ACT_CAUTION && m23 > 0.2f,
          "vote: tension votes wary");
    /* Exhaustion votes VETO (advisory — needs rule backing). */
    muse_brain_state_t b24;
    muse_brain_init(&b24, cap_log);
    b24.fatigue.stale = false;
    b24.fatigue.updated_ms = 1000;
    b24.fatigue.level = 0.95f;
    b24.fatigue.recovery_needed = true;
    CHECK(muse_brain_vote(&b24, NULL) == MUSE_ACT_VETO,
          "vote: exhaustion votes withdraw");

    /* Resolution: rules are hard constraints, the vote advises. */
    CHECK(muse_resolve_verdict(MUSE_GATE_CAUTION, MUSE_ACT_PROCEED, 0.5f) ==
          MUSE_GATE_PROCEED, "resolve: eager softens caution");
    CHECK(muse_resolve_verdict(MUSE_GATE_CAUTION, MUSE_ACT_PROCEED, 0.1f) ==
          MUSE_GATE_CAUTION, "resolve: weak eagerness doesn't soften");
    CHECK(muse_resolve_verdict(MUSE_GATE_VETO, MUSE_ACT_PROCEED, 1.0f) ==
          MUSE_GATE_VETO, "resolve: a rule veto never softens");
    CHECK(muse_resolve_verdict(MUSE_GATE_PROCEED, MUSE_ACT_CAUTION, 0.5f) ==
          MUSE_GATE_CAUTION, "resolve: wariness hardens proceed");
    CHECK(muse_resolve_verdict(MUSE_GATE_PROCEED, MUSE_ACT_VETO, 0.9f) ==
          MUSE_GATE_CAUTION, "resolve: the vote never vetoes alone");
    CHECK(muse_resolve_verdict(MUSE_GATE_PROCEED, MUSE_ACT_PROCEED, 0.0f) ==
          MUSE_GATE_PROCEED, "resolve: quiet rules, quiet vote");
    char snap20[1152];
    CHECK(muse_brain_snapshot(&b22, NULL, snap20, sizeof(snap20)) > 0 &&
          strstr(snap20, "\"vote\":\"proceed\"") != NULL,
          "vote: snapshot reports it");

    printf(failures ? "\n%d FAILURES\n" : "\nall brain tests passed\n", failures);
    return failures != 0;
}
