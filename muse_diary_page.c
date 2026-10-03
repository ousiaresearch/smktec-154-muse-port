/*
 * muse_diary_page.c — the diary reader settings page.
 */
#include "muse_diary_page.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "muse_diary.h"

#define DIARY_DIR "/sdcard/lapis/diary"
/* Show the tail of the day, not the whole archive. */
#define TAIL_BYTES 3072

static lv_obj_t *s_text;
static int s_shown_yday = -1;

/* Pure: take the tail of a buffer starting at a line boundary. */
static const char *tail_lines(const char *buf, size_t len, size_t cap)
{
    if (len <= cap)
        return buf;
    const char *p = buf + len - cap;
    const char *nl = memchr(p, '\n', cap);
    return nl ? nl + 1 : p;
}

static void load_day(void)
{
    time_t t = time(NULL);
    struct tm tm;
    char path[128];
    if (t < 1700000000 || !localtime_r(&t, &tm)) {
        lv_label_set_text(s_text, "The clock isn't set yet —\nentries are in undated.txt.");
        return;
    }
    s_shown_yday = tm.tm_yday;
    snprintf(path, sizeof(path), DIARY_DIR "/%04d-%02d-%02d.txt",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    if (!muse_diary_ready()) {
        lv_label_set_text(s_text, "No SD card — no diary yet.");
        return;
    }
    FILE *f = fopen(path, "r");
    if (!f) {
        lv_label_set_text(s_text, "Nothing written today yet.");
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    long want = size > (long)TAIL_BYTES ? (long)TAIL_BYTES : size;
    fseek(f, size - want, SEEK_SET);
    static char buf[TAIL_BYTES + 1];
    size_t n = fread(buf, 1, (size_t)want, f);
    fclose(f);
    buf[n] = '\0';
    const char *show = tail_lines(buf, n, TAIL_BYTES);
    lv_label_set_text(s_text, show[0] ? show : "Nothing written today yet.");
}

void muse_diary_page_build(lv_obj_t *list)
{
    s_text = lv_label_create(list);
    lv_obj_set_width(s_text, lv_pct(100));
    lv_label_set_long_mode(s_text, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_font(s_text, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_text, lv_color_hex(0xcccccc), 0);
    load_day();
}

void muse_diary_page_tick(void)
{
    time_t t = time(NULL);
    struct tm tm;
    if (t >= 1700000000 && localtime_r(&t, &tm) && tm.tm_yday != s_shown_yday)
        load_day();
}
