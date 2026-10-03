/*
 * muse_diary_page.h — the diary reader settings page.
 *
 * Shows today's diary entries from the SD card: the morning report's
 * source material, readable on the device itself. The private-dreams
 * rule applies here too — entries marked [private] are shown as-is on
 * the card (Anduril may always inspect the card), but the page never
 * pushes them anywhere.
 *
 * ESP-IDF draft — VERIFY on hardware: scroll performance, long-line
 * wrapping at 240px.
 */
#pragma once

#include "lvgl.h"

/* Build the page content into the settings list. */
void muse_diary_page_build(lv_obj_t *list);

/* Reload when the day rolls over; call when the page is on screen. */
void muse_diary_page_tick(void);
