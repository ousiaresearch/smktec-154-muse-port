/*
 * muse_ledger.h — the turn ledger.
 *
 * Every conversation turn lands on the SD card at
 * /sdcard/lapis/turns/YYYY-MM-DD.txt: what was heard, what was said,
 * whether it was delivered. The dream pass consolidates from this; the
 * diary keeps the story, the ledger keeps the receipts.
 *
 * The voice task calls these (via weak hooks patched into muse_voice.c);
 * the implementations below are the real ones. Best-effort like the
 * diary: the ledger never fails a turn.
 *
 * Privacy: transcripts live on the user's own SD card, next to the
 * diary. Nothing leaves the device.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

/* Latest transcript (overwrites; the final HEARD wins). */
void muse_ledger_heard(const char *text);

/* Reply chunk; appended, capped. */
void muse_ledger_reply(const char *text);

/* Flush the turn: one entry with heard + reply + outcome. */
void muse_ledger_turn_end(bool delivered, const char *note);

/* Pure formatting; host-testable. */
size_t muse_ledger_format(const char *heard, const char *reply,
                          bool delivered, const char *note,
                          char *out, size_t cap);
