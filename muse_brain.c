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
#include <math.h>

#define CLAMP01(x) ((x) < 0.f ? 0.f : (x) > 1.f ? 1.f : (x))

void muse_brain_init(muse_brain_state_t *b, muse_brain_log_fn log)
{
    memset(b, 0, sizeof(*b));
    b->somatic.energy = 0.5f;
    b->somatic.arousal = 0.5f;
    b->lc.alertness = 0.5f;
    b->na_temp = 0.5f;            /* Doya β starts balanced */
    b->pred_energy = 0.5f;
    b->pred_tension = 0.0f;
    for (int d = 0; d < MUSE_LEARN_DOMAINS; d++) {
        b->learn_fast[d] = 0.5f;
        b->learn_slow[d] = 0.5f;
    }
    b->log = log;
    /* Subsystems start stale: the brain is honest about knowing nothing yet. */
    b->scn.stale = b->somatic.stale = b->fatigue.stale = true;
    b->lc.stale = b->dmn.stale = b->hippocampus.stale = true;
}

static bool aged(uint32_t updated_ms, uint32_t now_ms, uint32_t budget_ms)
{
    return (now_ms - updated_ms) > budget_ms;
}

/* Raw homeostatic drive magnitude (Keramati & Gutkin): Euclidean norm
 * of the per-drive deviations from setpoint. Stale subsystems
 * contribute nothing — no invented needs. Shared by tick() (for the
 * valence differentiator) and muse_brain_drive() (the public API). */
static float raw_drive(const muse_brain_state_t *b)
{
    float hunger = 0, tired = 0, tense = 0;
    if (!b->somatic.stale)
        hunger = CLAMP01(MUSE_SETPOINT_ENERGY - b->somatic.energy);
    if (!b->fatigue.stale)
        tired = CLAMP01(b->fatigue.level - MUSE_SETPOINT_FATIGUE);
    if (!b->somatic.stale)
        tense = CLAMP01(b->somatic.tension - MUSE_SETPOINT_TENSION);
    float mag = sqrtf(hunger * hunger + tired * tired + tense * tense);
    return mag > 1.0f ? 1.0f : mag;
}

void muse_brain_tick(muse_brain_state_t *b, uint32_t now_ms, bool sleeping)
{
    /* ALMA layers, three clocks. Fast signals move in seconds... */
    b->somatic.tension *= MUSE_DECAY_FAST;
    b->somatic.arousal += (0.5f - b->somatic.arousal) * (1.0f - MUSE_DECAY_FAST);
    b->lc.alertness += (0.5f - b->lc.alertness) * (1.0f - MUSE_DECAY_FAST);
    b->novelty *= MUSE_DECAY_FAST;
    b->vta *= MUSE_DECAY_FAST;

    /* L2 valence (SYSTEMS.md): derived, never assigned.
     * v = −Δdrive/Δt × gain + event pulse. Drive falling feels good,
     * drive rising feels bad — regardless of absolute drive level.
     * The emotion quadrant is Joffily's: sign(improvement velocity) ×
     * sign(improvement acceleration); sign flips name relief and
     * disappointment. */
    if (!b->somatic.stale) {
        float d = raw_drive(b);
        b->affect_pulse *= MUSE_DECAY_MEDIUM;
        float v_prev = b->somatic.valence;
        float v;
        if (!b->drive_seeded) {
            /* First fresh tick: seed the differentiator, no feeling yet. */
            b->drive_prev = d;
            b->drive_seeded = true;
            v = b->affect_pulse;
        } else {
            float v_body = -(d - b->drive_prev) * MUSE_VALENCE_GAIN;
            b->drive_prev = d;
            v = v_body + b->affect_pulse;
        }
        b->somatic.valence = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);

        const float DB = MUSE_EMO_DEADBAND;
        float dv = b->somatic.valence - v_prev;
        muse_emotion_t e = MUSE_EMO_CALM;
        if (v_prev > DB && b->somatic.valence < -DB)
            e = MUSE_EMO_DISAPPOINTMENT;
        else if (v_prev < -DB && b->somatic.valence > DB)
            e = MUSE_EMO_RELIEF;
        else if (b->somatic.valence > DB)
            e = (dv > 0) ? MUSE_EMO_HOPE : MUSE_EMO_HAPPINESS;
        else if (b->somatic.valence < -DB)
            e = (dv < 0) ? MUSE_EMO_FEAR : MUSE_EMO_UNHAPPINESS;
        b->emotion = e;
    } else {
        /* Stale body: no derived feeling; the old one fades. Re-seed
         * the differentiator on the next fresh tick so stale gaps don't
         * invent a velocity. */
        b->somatic.valence *= MUSE_DECAY_FAST;
        b->affect_pulse *= MUSE_DECAY_MEDIUM;
        b->emotion = MUSE_EMO_CALM;
        b->drive_seeded = false;
    }
    /* L6 boredom (Yu et al. HHVG): the anti-darkroom drive.
     * Boredom is not low stimulation — it is devaluation of the known:
     * familiarity with the current situation × (1 − recent information
     * gain). High boredom lowers the exploration temperature (Doya β)
     * so the creature seeks novelty instead of looping. */
    {
        int q = (!b->scn.stale && b->scn.quiet_hours) ? 1 : 0;
        int m = b->somatic.tension > 0.6f ? 2 :
                (b->somatic.tension > 0.25f ? 1 : 0);
        int r = (b->last_interaction_ms != 0 &&
                 now_ms - b->last_interaction_ms < 300000) ? 1 : 0;
        int ctx = (q << 3) | (m << 1) | r;
        b->familiarity[ctx] += (1.0f - b->familiarity[ctx]) * 0.002f;
        b->info_gain += (b->vta - b->info_gain) * (1.0f - MUSE_DECAY_MEDIUM);
        b->boredom = b->familiarity[ctx] * (1.0f - CLAMP01(b->info_gain));
        if (b->boredom > b->peak_boredom)
            b->peak_boredom = b->boredom;
        /* Boredom pulls β toward exploration; β relaxes back to 0.5. */
        float target = 0.5f - 0.5f * b->boredom;
        b->na_temp += (target - b->na_temp) * (1.0f - MUSE_DECAY_MEDIUM);
        /* Restlessness: crossing into boredom is a wandering episode. */
        if (b->prev_boredom <= 0.6f && b->boredom > 0.6f) {
            b->dmn.wanders++;
            b->dmn.updated_ms = now_ms;
            b->dmn.stale = false;
        }
        b->prev_boredom = b->boredom;
    }
    /* Mood ω (Hesp level-2 / Joffily eq. 4): slow EMA of derived
     * valence — signed model-fitness. This is what persists across
     * sleep and what gates learning in step 2. */
    b->somatic.mood += (b->somatic.valence - b->somatic.mood) *
                       (1.0f - MUSE_DECAY_SLOW);

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
    float energy = CLAMP01((voltage_v - 3.0f) / 1.2f);
    if (charging)
        energy = CLAMP01(energy + 0.05f);
    /* Curiosity as prediction error (Pathak, firmware scale): surprise is
     * how wrong the energy predictor was, and it drives vta. The
     * predictor itself learns at the valence-gated rate. */
    float err = fabsf(energy - b->pred_energy);
    b->pred_energy += (energy - b->pred_energy) * muse_brain_lr_eff(b, 0.1f);
    b->vta = CLAMP01(b->vta * 0.85f + err * 0.9f);
    b->somatic.energy = energy;
    b->somatic.updated_ms = now_ms;
    b->somatic.stale = false;
    b->fatigue.updated_ms = now_ms;   /* energy informs fatigue honesty */
}

void muse_brain_feed_motion(muse_brain_state_t *b, float agitation_01,
                            uint32_t now_ms)
{
    agitation_01 = CLAMP01(agitation_01);
    float err = fabsf(agitation_01 - b->pred_tension);
    b->pred_tension += (agitation_01 - b->pred_tension) *
                       muse_brain_lr_eff(b, 0.1f);
    b->vta = CLAMP01(b->vta * 0.85f + err * 0.9f);
    b->somatic.tension = CLAMP01(b->somatic.tension * 0.7f + agitation_01 * 0.3f);
    b->somatic.arousal = CLAMP01(b->somatic.arousal + agitation_01 * 0.2f);
    b->lc.alertness = CLAMP01(b->lc.alertness + agitation_01 * 0.25f);
    b->somatic.updated_ms = now_ms;
    b->somatic.stale = false;
    b->lc.updated_ms = now_ms;
    b->lc.stale = false;
    b->dmn.rest_ms = 0;               /* motion ends rest */
}

void muse_brain_feed_valence(muse_brain_state_t *b, float delta,
                             uint32_t now_ms)
{
    /* Affect event: pet +, error −. Adds a transient pulse; the tick
     * folds it into derived valence. This is the social-coupling
     * channel (things that move the creature outside the drive model),
     * not a valence assignment. */
    float p = b->affect_pulse + delta;
    b->affect_pulse = p > 1.0f ? 1.0f : (p < -1.0f ? -1.0f : p);
    b->somatic.updated_ms = now_ms;
    b->somatic.stale = false;
}

const char *muse_emotion_name(muse_emotion_t e)
{
    switch (e) {
    case MUSE_EMO_HOPE:           return "hope";
    case MUSE_EMO_HAPPINESS:      return "happiness";
    case MUSE_EMO_FEAR:           return "fear";
    case MUSE_EMO_UNHAPPINESS:    return "unhappiness";
    case MUSE_EMO_RELIEF:         return "relief";
    case MUSE_EMO_DISAPPOINTMENT: return "disappointment";
    default:                      return "calm";
    }
}

float muse_brain_lr_eff(const muse_brain_state_t *b, float base_lr)
{
    float m = expf(-MUSE_LR_VALENCE_K * b->somatic.valence +
                   b->somatic.mood);
    if (m < 0.2f) m = 0.2f;
    if (m > 4.0f) m = 4.0f;
    return base_lr * m;
}

void muse_brain_feed_learning(muse_brain_state_t *b, int domain,
                              float success01, uint32_t now_ms)
{
    /* Learning progress (Oudeyer): progress = fast EMA − slow EMA.
     * Both EMAs learn at the valence-gated rate — in a failing world
     * even the baseline lets go faster. */
    if (domain < 0 || domain >= MUSE_LEARN_DOMAINS)
        return;
    (void)now_ms;
    success01 = CLAMP01(success01);
    b->learn_fast[domain] += (success01 - b->learn_fast[domain]) *
                             muse_brain_lr_eff(b, 0.3f);
    b->learn_slow[domain] += (success01 - b->learn_slow[domain]) *
                             muse_brain_lr_eff(b, 0.05f);
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

float muse_brain_drive(const muse_brain_state_t *b, char *need_out,
                       size_t need_n)
{
    /* Homeostatic drive (Keramati & Gutkin): deviation from setpoint.
     * Stale subsystems contribute nothing — no invented needs. */
    float hunger = 0, tired = 0, tense = 0;
    if (!b->somatic.stale)
        hunger = CLAMP01(MUSE_SETPOINT_ENERGY - b->somatic.energy);
    if (!b->fatigue.stale)
        tired = CLAMP01(b->fatigue.level - MUSE_SETPOINT_FATIGUE);
    if (!b->somatic.stale)
        tense = CLAMP01(b->somatic.tension - MUSE_SETPOINT_TENSION);

    const char *need = "none";
    float biggest = 0.0f;
    if (hunger > biggest) { biggest = hunger; need = "hunger"; }
    if (tired > biggest)  { biggest = tired;  need = "rest"; }
    if (tense > biggest)  { biggest = tense;  need = "calm"; }

    /* Euclidean magnitude of the deviation vector (raw_drive). */
    float mag = raw_drive(b);
    if (need_out && need_n > 0) {
        strncpy(need_out, biggest > 0.05f ? need : "none", need_n - 1);
        need_out[need_n - 1] = '\0';
    }
    return mag;
}

muse_gut_t muse_brain_suggest_gut(const muse_brain_state_t *b)
{
    /* Appraisal in Scherer's sequential-check order (EMA lineage):
     * 1. suddenness/novelty, 2. intrinsic pleasantness,
     * 3. goal conduciveness, 4. coping potential. */
    const bool stale = b->somatic.stale;
    const float energy  = stale ? 0.5f : b->somatic.energy;
    const float valence = stale ? 0.0f : b->somatic.valence;
    const float tension = stale ? 0.0f : b->somatic.tension;
    const float fatigue = b->fatigue.stale ? 0.0f : b->fatigue.level;
    const float surprise = b->vta;

    /* 1. Suddenness: high surprise with low coping -> orient, don't act. */
    if (surprise > 0.7f && fatigue > 0.5f)
        return MUSE_GUT_WAIT;
    /* 2. Intrinsic pleasantness: deeply bad + no resources -> stop. */
    if (valence < -0.8f && energy < 0.25f)
        return MUSE_GUT_STOP;
    if (valence < -0.6f)
        return MUSE_GUT_DOUBT;
    /* 3. Goal conduciveness: a critical need comes before anything else. */
    if (energy < 0.35f)
        return MUSE_GUT_PAUSE;
    /* 4. Coping potential: can I handle this right now? */
    if (fatigue > 0.7f)
        return MUSE_GUT_PAUSE;
    if (tension > 0.8f)
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
    out->boredom = b->boredom;
    out->explore_temp = b->na_temp;
    out->adj_confidence = CLAMP01(adj_confidence);
    out->recovery_needed = !b->fatigue.stale && b->fatigue.recovery_needed;
    out->distracted = b->distracted;
    out->quiet_hours = !b->scn.stale && b->scn.quiet_hours;
    out->gut = gut;
    out->drive = out->recovery_needed ? "recovery" : NULL;
}

void muse_brain_learning_save(const muse_brain_state_t *b, muse_identity_t *id)
{
    for (int d = 0; d < MUSE_LEARN_DOMAINS; d++)
        id->learn_base[d] = b->learn_slow[d];
}

void muse_brain_learning_restore(muse_brain_state_t *b,
                                 const muse_identity_t *id)
{
    for (int d = 0; d < MUSE_LEARN_DOMAINS; d++) {
        b->learn_fast[d] = id->learn_base[d];
        b->learn_slow[d] = id->learn_base[d];
    }
}

void muse_brain_consolidate(muse_brain_state_t *b, muse_identity_t *id)
{
    /* Care-day: a day with real interaction. Missed days don't punish;
     * they just don't advance. The window is not the door. */
    if (b->interactions >= MUSE_CARE_DAY_INTERACTIONS)
        id->care_days++;

    /* Learning progress (Oudeyer 2007): sustained improvement earns
     * mastery credits, which count (capped) toward growth alongside
     * care-days. Development follows mastery, not just attendance. */
    float lp = 0.0f;
    for (int d = 0; d < MUSE_LEARN_DOMAINS; d++) {
        float p = b->learn_fast[d] - b->learn_slow[d];
        if (p > 0.0f) lp += p;
    }
    if (lp > 0.3f && id->mastery < 4) {
        id->mastery++;
        if (b->log) b->log("mastery: sustained learning progress");
    }

    /* Growth: the child earns stages, never buys them. */
    static const uint32_t marks[MUSE_GROWTH_STAGES] = { 0, 2, 5, 12 };
    static const char *milestones[MUSE_GROWTH_STAGES] = {
        "",
        "growth: stage 1 (toddler) — first words are coming; the 'no' phase begins",
        "growth: stage 2 (child) — it asks why now",
        "growth: stage 3 (adolescent) — it has opinions about itself",
    };
    uint32_t effective = id->care_days + (id->mastery > 2 ? 2 : id->mastery);
    uint32_t want = 0;
    for (uint32_t s = 1; s < MUSE_GROWTH_STAGES; s++)
        if (effective >= marks[s])
            want = s;
    if (want > id->growth_stage) {
        id->growth_stage = want;
        if (b->log)
            b->log(milestones[want]);
    }

    /* The dream pass: fold the day's counters into one diary entry. */
    if (b->log) {
        char line[256];
        snprintf(line, sizeof(line),
                 "dream: %lu interactions, %lu vetoes, %lu cautions, "
                 "peak fatigue %.2f, peak boredom %.2f, %lu novel encounters%s%s",
                 (unsigned long)b->interactions,
                 (unsigned long)b->veto_count,
                 (unsigned long)b->caution_count,
                 (double)b->peak_fatigue,
                 (double)b->peak_boredom,
                 (unsigned long)b->novel_count,
                 b->veto_count ? ", last veto: " : "",
                 b->veto_count ? b->last_veto_reason : "");
        b->log(line);
    }
    b->hippocampus.entries++;
    /* The capability baseline ratchets: tomorrow's progress is measured
     * against what the child can already do. */
    muse_brain_learning_save(b, id);
    /* Reset the day counters; the diary keeps what happened. */
    b->interactions = 0;
    b->veto_count = 0;
    b->caution_count = 0;
    b->peak_fatigue = 0;
    b->peak_boredom = 0;
    b->novel_count = 0;
    b->last_veto_reason[0] = '\0';
}

size_t muse_brain_snapshot(const muse_brain_state_t *b,
                           const muse_identity_t *id,
                           char *out, size_t out_n)
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
    char need[16];
    float drive = muse_brain_drive(b, need, sizeof(need));
    int n = snprintf(out, out_n,
        "{\"self\":\"%.31s\",\"gen\":%lu,\"stage\":%lu,"
        "\"energy\":%.2f,\"tension\":%.2f,\"arousal\":%.2f,\"valence\":%.2f,"
        "\"emotion\":\"%s\",\"boredom\":%.2f,"
        "\"mood\":%.2f,\"drive\":%.2f,\"need\":\"%s\","
        "\"fatigue\":%.2f,\"quiet\":%s,\"phase\":%.2f,"
        "\"gate\":\"%s\",\"gate_why\":\"%s\","
        "\"interactions\":%lu,\"stale\":[%s]}",
        id && id->name[0] ? id->name : "?",
        (unsigned long)(id ? id->generation : 0),
        (unsigned long)(id ? id->growth_stage : 0),
        b->somatic.stale ? -1.0 : (double)b->somatic.energy,
        b->somatic.stale ? -1.0 : (double)b->somatic.tension,
        b->lc.stale ? -1.0 : (double)b->lc.alertness,
        b->somatic.stale ? -9.0 : (double)b->somatic.valence,
        muse_emotion_name(b->emotion),
        (double)b->boredom,
        b->somatic.stale ? -9.0 : (double)b->somatic.mood,
        (double)drive, need,
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
