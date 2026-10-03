/*
 * muse_vitals_page.h — the vitals / body-map settings page.
 *
 * The biomimetic-brain rule, rendered: every subsystem shows on a
 * screen. Four bars (energy, tension, mood, fatigue), a three-region
 * body map (head = alertness, chest = energy, gut = drive), and three
 * lines (want, gate verdict, growth stage).
 *
 * The SDK's settings UI owns the page shell (see the apply.sh patch);
 * this file builds the content into the list it hands over and refreshes
 * it on tick. LVGL calls happen on the LVGL task only.
 *
 * ESP-IDF draft — VERIFY on hardware: layout at 240x240, bar colors,
 * refresh cost.
 */
#pragma once

#include "lvgl.h"

/* Build the page content into the settings list. */
void muse_vitals_page_build(lv_obj_t *list);

/* Refresh from the live brain; call when the page is on screen. */
void muse_vitals_page_tick(void);
