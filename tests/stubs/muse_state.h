#pragma once
#include <stdarg.h>
/* Test stub: caption + verdict live in stub_fns.c (single TU). */
void muse_state_set_caption(const char *fmt, ...);
const char *stub_caption_seen(void);
