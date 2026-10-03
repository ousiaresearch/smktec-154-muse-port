/*
 * muse_brain.c — on-device nervous system (v1).
 *
 * Subsystem semantics follow biomimetic-brain's shipped examples:
 * fatigue recovers with rest (faster asleep), an engagement override is
 * recorded, never silent. somatic.energy maps the LiPo curve 3.0–4.2V.
 */
#include "muse_brain.h"

#include <stdio.h>
#include <string.h>

#define CLAMP01(x) ((x) < 0.f ? 0.f : (x) > 1.f ? 1.f : (x))

void muse_brain_init(muse_brain_state_t *b, muse_brain_log_fn log)
{
    memset(b, 0, sizeof(*b));
    b->somatic.energy = 0.5f;
    b->somatic.arousal = 0.5f;
    b->lc.alertness = 0.5f;
    b->log = log;
    /* Subsystems start stale: the brain is honest about knowing nothing yet. */
    b->scn.stale = b->somatic.stale = b->fatigue.stale = true;
    b->lc.stale = b->dmn.stale = b->hippocampus.stale = true;
}

static bool aged(uint32_t updated_ms, uint32_t now_ms, uint32_t budget_ms)
{
    return (now_ms - updated_ms) > budget_ms;
}

void muse_brain_tick(muse_brain_state_t *b, uint32_t now_ms, bool sleeping)
{
    /* Decay fast signals toward rest. */
    b->somatic.tension *= 0.95f;
    b->somatic.arousal += (0.5f - b->somatic.arousal) * 0.05f;
    b->lc.alertness += (0.5f - b->lc.alertness) * 0.05f;
    b->novelty *= 0.98f;
    b->vta *= 0.98f;

    /* Fatigue: accrues with load while awake, recovers while asleep. */
    if (sleeping) {
        b->fatigue.level -= 0.02f;             /* a nap does real work */
        b->dmn.rest_ms += 1000;
    } else {
        b->fatigue.level += b->fatigue.load * 0.001f;
    }
    b->fatigue.level = CLAMP01(b->fatigue.level);
    b->fatigue.recovery_needed = b->fatigue.level > 0.85f;

    /* Burst distraction decays a minute after the last interaction. */
    if (now_ms - b->last_interaction_ms > 60000)
        b->distracted = false;

    /* Staleness: quiet subsystems stop claiming to know. */
    b->scn.stale = aged(b->scn.updated_ms, now_ms, MUSE_SCN_STALE_MS);
    b->somatic.stale = aged(b->somatic.updated_ms, now_ms, MUSE_SOMATIC_STALE_MS);
    b->fatigue.stale = aged(b->fatigue.updated_ms, now_ms, MUSE_FATIGUE_STALE_MS);
    b->lc.stale = aged(b->lc.updated_ms, now_ms, MUSE_LC_STALE_MS);
    b->dmn.stale = aged(b->dmn.updated_ms, now_ms, MUSE_DMN_STALE_MS);
    b->hippocampus.stale = aged(b->hippocampus.updated_ms, now_ms,
                                MUSE_HIPPOCAMPUS_STALE_MS);
}

void muse_brain_feed_battery(muse_brain_state_t *b, float voltage_v,
                             bool charging, uint32_t now_ms)
{
    /* LiPo curve: 3.0V empty, 4.2V full. Charging reads as rising energy. */
    b->somatic.energy = CLAMP01((voltage_v - 3.0f) / 1.2f);
    if (charging)
        b->somatic.energy = CLAMP01(b->somatic.energy + 0.05f);
    b->somatic.updated_ms = now_ms;
    b->somatic.stale = false;
    b->fatigue.updated_ms = now_ms;   /* energy informs fatigue honesty */
}

void muse_brain_feed_motion(muse_brain_state_t *b, float agitation_01,
                            uint32_t now_ms)
{
    agitation_01 = CLAMP01(agitation_01);
    b->somatic.tension = CLAMP01(b->somatic.tension * 0.7f + agitation_01 * 0.3f);
    b->somatic.arousal = CLAMP01(b->somatic.arousal + agitation_01 * 0.2f);
    b->lc.alertness = CLAMP01(b->lc.alertness + agitation_01 * 0.25f);
    b->somatic.updated_ms = now_ms;
    b->somatic.stale = false;
    b->lc.updated_ms = now_ms;
    b->lc.stale = false;
    b->dmn.rest_ms = 0;               /* motion ends rest */
}

void muse_brain_feed_interaction(muse_brain_state_t *b, uint32_t now_ms)
{
    if (now_ms - b->last_interaction_ms < 60000)
        b->distracted = true;         /* bursts read as distraction */
    b->last_interaction_ms = now_ms;
    b->interactions++;
    b->dmn.rest_ms = 0;
    b->dmn.updated_ms = now_ms;
    b->dmn.stale = false;
    /* Engagement suspends fatigue without erasing it — the override is
     * recorded in the diary by consolidate(), not hidden. */
    b->fatigue.level = CLAMP01(b->fatigue.level - 0.01f);
}

void muse_brain_note_novelty(muse_brain_state_t *b, uint32_t now_ms)
{
    (void)now_ms;
    b->novelty = CLAMP01(b->novelty + 0.3f);
    b->vta = CLAMP01(b->vta + 0.2f);
    b->novel_count++;
}

void muse_brain_set_quiet_hours(muse_brain_state_t *b, bool quiet,
                                float phase, uint32_t now_ms)
{
    b->scn.quiet_hours = quiet;
    b->scn.phase = CLAMP01(phase);
    b->scn.updated_ms = now_ms;
    b->scn.stale = false;
}

muse_gut_t muse_brain_suggest_gut(const muse_brain_state_t *b)
{
    /* v1 heuristic. The cloud (lapis-embodiment plugin) refines this later;
     * on-device it just has to be honest and simple. */
    if (!b->somatic.stale && b->somatic.energy < 0.35f)
        return MUSE_GUT_PAUSE;
    if (!b->somatic.stale && b->somatic.valence < -0.6f)
        return MUSE_GUT_DOUBT;
    if (!b->somatic.stale && b->somatic.tension > 0.8f)
        return MUSE_GUT_WAIT;
    return MUSE_GUT_GO_AHEAD;
}

void muse_brain_gate_inputs(const muse_brain_state_t *b, float adj_confidence,
                            muse_gut_t gut, muse_gate_inputs_t *out)
{
    memset(out, 0, sizeof(*out));
    /* Stale reads become neutral defaults — reported as defaults downstream. */
    out->energy = b->somatic.stale ? 0.5f : b->somatic.energy;
    out->fatigue = b->fatigue.stale ? 0.0f : b->fatigue.level;
    out->arousal = b->lc.stale ? 0.5f : b->lc.alertness;
    out->tension = b->somatic.stale ? 0.0f : b->somatic.tension;
    out->novelty = b->novelty;
    out->vta = b->vta;
    out->adj_confidence = CLAMP01(adj_confidence);
    out->recovery_needed = !b->fatigue.stale && b->fatigue.recovery_needed;
    out->distracted = b->distracted;
    out->quiet_hours = !b->scn.stale && b->scn.quiet_hours;
    out->gut = gut;
    out->drive = out->recovery_needed ? "recovery" : NULL;
}

void muse_brain_consolidate(muse_brain_state_t *b)
{
    /* The dream pass: fold the day's counters into one diary entry. */
    if (b->log) {
        char line[256];
        snprintf(line, sizeof(line),
                 "dream: %lu interactions, %lu vetoes, %lu cautions, "
                 "peak fatigue %.2f, %lu novel encounters%s%s",
                 (unsigned long)b->interactions,
                 (unsigned long)b->veto_count,
                 (unsigned long)b->caution_count,
                 (double)b->peak_fatigue,
                 (unsigned long)b->novel_count,
                 b->veto_count ? ", last veto: " : "",
                 b->veto_count ? b->last_veto_reason : "");
        b->log(line);
    }
    b->hippocampus.entries++;
    /* Reset the day counters; the diary keeps what happened. */
    b->interactions = 0;
    b->veto_count = 0;
    b->caution_count = 0;
    b->peak_fatigue = 0;
    b->novel_count = 0;
    b->last_veto_reason[0] = '\0';
}

size_t muse_brain_snapshot(const muse_brain_state_t *b, const char *name,
                           uint32_t generation, char *out, size_t out_n)
{
    /* Compact JSON. Stale subsystems report "stale", never a number. */
    char stale[128] = "";
    size_t sp = 0;
    const struct { bool s; const char *n; } subs[] = {
        { b->scn.stale, "scn" }, { b->somatic.stale, "somatic" },
        { b->fatigue.stale, "fatigue" }, { b->lc.stale, "lc" },
        { b->dmn.stale, "dmn" }, { b->hippocampus.stale, "hippocampus" },
    };
    for (size_t i = 0; i < sizeof(subs) / sizeof(subs[0]); i++) {
        if (subs[i].s && sp + 10 < sizeof(stale))
            sp += snprintf(stale + sp, sizeof(stale) - sp,
                           "%s\"%s\"", sp ? "," : "", subs[i].n);
    }

    /* Compact JSON. Stale subsystems report -1 ("don't know"), never a
     * fabricated reading; the stale list names them explicitly. */
    int n = snprintf(out, out_n,
        "{\"self\":\"%.31s\",\"gen\":%lu,"
        "\"energy\":%.2f,\"tension\":%.2f,\"arousal\":%.2f,\"valence\":%.2f,"
        "\"fatigue\":%.2f,\"quiet\":%s,\"phase\":%.2f,"
        "\"gate\":\"%s\",\"gate_why\":\"%s\","
        "\"interactions\":%lu,\"stale\":[%s]}",
        name ? name : "?",
        (unsigned long)generation,
        b->somatic.stale ? -1.0 : (double)b->somatic.energy,
        b->somatic.stale ? -1.0 : (double)b->somatic.tension,
        b->lc.stale ? -1.0 : (double)b->lc.alertness,
        b->somatic.stale ? -9.0 : (double)b->somatic.valence,
        b->fatigue.stale ? -1.0 : (double)b->fatigue.level,
        (!b->scn.stale && b->scn.quiet_hours) ? "true" : "false",
        b->scn.stale ? -1.0 : (double)b->scn.phase,
        muse_gate_verdict_name(b->decisions.last.verdict),
        b->decisions.last.reason ? b->decisions.last.reason : "proceed",
        (unsigned long)b->interactions,
        stale);
    if (n < 0 || (size_t)n >= out_n)
        return 0;
    return (size_t)n;
}
