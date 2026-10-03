/* Stubs for the diary/state/pixel calls the turn gate makes. Single TU. */
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static char s_diary_last[256];
void muse_diary_append(const char *line) {
    if (line) { strncpy(s_diary_last, line, sizeof(s_diary_last) - 1); }
}
const char *stub_diary_last_line(void) { return s_diary_last; }

static char s_caption[128];
void muse_state_set_caption(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(s_caption, sizeof(s_caption), fmt, ap);
    va_end(ap);
}
const char *stub_caption_seen(void) { return s_caption; }

static int s_verdict = -1;
void muse_pixel_set_verdict(int v) { s_verdict = v; }
int stub_verdict_seen(void) { return s_verdict; }

static bool s_diary_ready = false;
void muse_diary_set_ready(bool r) { s_diary_ready = r; }
bool muse_diary_ready(void) { return s_diary_ready; }
