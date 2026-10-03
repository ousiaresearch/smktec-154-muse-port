/*
 * muse_ledger.c — the turn ledger.
 *
 * ESP-IDF draft — builds on the Mac against the IDF. Not host-testable
 * (needs the diary's SD mount); the pure formatting is covered by the
 * host tests in tests/test_ledger.c.
 */
#include "muse_ledger.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "esp_log.h"

#include "muse_diary.h"

static const char *TAG = "ledger";

#define LEDGER_DIR "/sdcard/lapis/turns"
#define HEARD_CAP  512
#define REPLY_CAP  2048

static char s_heard[HEARD_CAP];
static char s_reply[REPLY_CAP];
static size_t s_reply_len;

/* Pure: format one ledger entry. Host-testable. */
size_t muse_ledger_format(const char *heard, const char *reply,
                          bool delivered, const char *note,
                          char *out, size_t cap)
{
    char outcome[96];
    if (note && note[0])
        snprintf(outcome, sizeof(outcome), "%s (%s)",
                 delivered ? "delivered" : "failed", note);
    else
        snprintf(outcome, sizeof(outcome), "%s",
                 delivered ? "delivered" : "failed");
    int n = snprintf(out, cap, "heard: %.400s\nreply: %.1500s\noutcome: %s\n---\n",
                     heard ? heard : "",
                     reply ? reply : "",
                     outcome);
    if (n < 0 || (size_t)n >= cap)
        return 0;
    return (size_t)n;
}

void muse_ledger_heard(const char *text)
{
    if (!text)
        return;
    strncpy(s_heard, text, sizeof(s_heard) - 1);
    s_heard[sizeof(s_heard) - 1] = '\0';
    s_reply[0] = '\0';
    s_reply_len = 0;
}

void muse_ledger_reply(const char *text)
{
    if (!text)
        return;
    size_t n = strlen(text);
    size_t room = REPLY_CAP - 1 - s_reply_len;
    if (room == 0)
        return;
    if (n > room)
        n = room;
    memcpy(s_reply + s_reply_len, text, n);
    s_reply_len += n;
    s_reply[s_reply_len] = '\0';
}

void muse_ledger_turn_end(bool delivered, const char *note)
{
    if (!muse_diary_ready())
        goto reset;
    mkdir(LEDGER_DIR, 0755);   /* diary init made /sdcard/lapis; ignore EEXIST */

    time_t t = time(NULL);
    struct tm tm;
    char path[128];
    if (t < 1700000000 || !localtime_r(&t, &tm))
        snprintf(path, sizeof(path), LEDGER_DIR "/undated.txt");
    else
        snprintf(path, sizeof(path), LEDGER_DIR "/%04d-%02d-%02d.txt",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    FILE *f = fopen(path, "a");
    if (!f) {
        ESP_LOGW(TAG, "ledger append failed");
        goto reset;
    }
    char entry[HEARD_CAP + REPLY_CAP + 128];
    char stamp[32];
    if (t >= 1700000000 && localtime_r(&t, &tm))
        snprintf(stamp, sizeof(stamp), "[%02d:%02d] ", tm.tm_hour, tm.tm_min);
    else
        snprintf(stamp, sizeof(stamp), "[clock-unset] ");
    size_t n = muse_ledger_format(s_heard[0] ? s_heard : NULL,
                                  s_reply_len ? s_reply : NULL,
                                  delivered, note, entry, sizeof(entry));
    if (n)
        fprintf(f, "%s%.*s", stamp, (int)n, entry);
    fclose(f);

reset:
    s_heard[0] = '\0';
    s_reply[0] = '\0';
    s_reply_len = 0;
}
