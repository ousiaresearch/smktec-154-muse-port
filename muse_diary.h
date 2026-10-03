/*
 * muse_diary.h — the SD-card diary sink.
 *
 * The brain emits plain-text dream lines via its log callback; this module
 * appends them to /sdcard/lapis/diary/YYYY-MM-DD.txt. The muse dreams, and
 * the card holds the dreams. Plain text, user-owned, no cloud copy.
 *
 * Mount is best-effort: no card (or a mount failure) degrades to a log
 * line, never a boot failure.
 *
 * ESP-IDF draft — builds on the Mac. VERIFY: the TF slot's SDMMC wiring
 * against the real board (default slot-1 pins assumed here).
 */
#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mount the card and prepare the diary directory. Safe to call once. */
esp_err_t muse_diary_init(void);

/* Append one line (timestamped) to today's diary file. No-op if unmounted. */
void muse_diary_append(const char *line);

/* True once a card is mounted and writable. */
bool muse_diary_ready(void);

#ifdef __cplusplus
}
#endif
