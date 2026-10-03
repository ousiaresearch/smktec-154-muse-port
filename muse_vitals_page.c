/*
 * muse_vitals_page.c — the vitals / body-map settings page.
 */
#include "muse_vitals_page.h"

#include <stdio.h>
#include <string.h>

#include "muse_brain.h"
#include "muse_gate.h"
#include "muse_turn_gate.h"

/* Board accessors (board_smktec_s3_touch_lcd_154.c). */
extern muse_brain_state_t *smktec_brain(void);
extern muse_identity_t *smktec_identity(void);

static lv_obj_t *s_bars[4];
static lv_obj_t *s_map[3];
static lv_obj_t *s_want, *s_gate, *s_stage;

static const char *const BAR_NAMES[4] = { "ENERGY", "TENSION", "MOOD", "FATIGUE" };
static const uint32_t BAR_COLORS[4] = { 0x3fd9a0, 0xffb020, 0x5cb8ff, 0xb08cff };
static const char *const MAP_NAMES[3] = { "HEAD", "CHEST", "GUT" };
static const char *const STAGE_NAMES[4] = { "newborn", "toddler", "child", "adolescent" };

/* One horizontal row: label left, bar filling the rest. */
static lv_obj_t *bar_row(lv_obj_t *list, const char *name, uint32_t color, int idx)
{
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), 34);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);

    lv_obj_t *l = lv_label_create(row);
    lv_label_set_text(l, name);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x9a9a9a), 0);
    lv_obj_set_width(l, 74);

    lv_obj_t *bar = lv_bar_create(row);
    lv_obj_set_height(bar, 12);
    lv_obj_set_flex_grow(bar, 1);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    s_bars[idx] = bar;
    return row;
}

static lv_obj_t *info_line(lv_obj_t *list)
{
    lv_obj_t *l = lv_label_create(list);
    lv_label_set_text(l, "");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xcccccc), 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(l, lv_pct(100));
    return l;
}

void muse_vitals_page_build(lv_obj_t *list)
{
    for (int i = 0; i < 4; i++)
        bar_row(list, BAR_NAMES[i], BAR_COLORS[i], i);

    /* Body map: three vertical bars — head (alertness), chest (energy),
     * gut (drive). The body, at a glance. */
    lv_obj_t *map = lv_obj_create(list);
    lv_obj_remove_style_all(map);
    lv_obj_set_size(map, lv_pct(100), 108);
    lv_obj_set_flex_flow(map, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(map, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(map, 18, 0);
    for (int i = 0; i < 3; i++) {
        lv_obj_t *col = lv_obj_create(map);
        lv_obj_remove_style_all(col);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *bar = lv_bar_create(col);
        lv_obj_set_size(bar, 22, 72);   /* taller than wide: vertical */
        lv_bar_set_range(bar, 0, 100);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, lv_color_hex(BAR_COLORS[i == 2 ? 1 : i]), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
        s_map[i] = bar;
        lv_obj_t *l = lv_label_create(col);
        lv_label_set_text(l, MAP_NAMES[i]);
        lv_obj_set_style_text_font(l, &lv_font_unscii_16, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0x9a9a9a), 0);
    }

    s_want = info_line(list);
    s_gate = info_line(list);
    s_stage = info_line(list);
}

void muse_vitals_page_tick(void)
{
    muse_brain_state_t *b = smktec_brain();
    muse_identity_t *id = smktec_identity();
    if (!b || !id)
        return;

    bool warming = b->somatic.stale || b->fatigue.stale || b->lc.stale;
    float energy = b->somatic.stale ? 0 : b->somatic.energy;
    float tension = b->somatic.stale ? 0 : b->somatic.tension;
    float mood = b->somatic.stale ? 0 : (b->somatic.mood + 1.0f) / 2.0f;
    float fatigue = b->fatigue.stale ? 0 : b->fatigue.level;
    float alert = b->lc.stale ? 0 : b->lc.alertness;
    char need[16];
    float drive = muse_brain_drive(b, need, sizeof(need));

    float bars[4] = { energy * 100, tension * 100, mood * 100, fatigue * 100 };
    for (int i = 0; i < 4; i++)
        lv_bar_set_value(s_bars[i], (int32_t)bars[i], LV_ANIM_OFF);
    float mapv[3] = { alert * 100, energy * 100, drive * 100 };
    for (int i = 0; i < 3; i++)
        lv_bar_set_value(s_map[i], (int32_t)mapv[i], LV_ANIM_OFF);

    char buf[96];
    if (warming)
        snprintf(buf, sizeof(buf), "warming up — sensors settling");
    else
        snprintf(buf, sizeof(buf), "want: %s (%.2f)", need, drive);
    lv_label_set_text(s_want, buf);

    snprintf(buf, sizeof(buf), "gate: %s (%s)",
             muse_gate_verdict_name(muse_turn_gate_verdict()),
             muse_turn_gate_reason());
    lv_label_set_text(s_gate, buf);

    uint32_t st = id->growth_stage < 4 ? id->growth_stage : 3;
    snprintf(buf, sizeof(buf), "stage: %s · day %lu",
             STAGE_NAMES[st], (unsigned long)id->care_days);
    lv_label_set_text(s_stage, buf);
}
