/* Host test for muse_ledger.c formatting (the SD writing itself needs
 * the IDF; the pure format function is tested here).
 */
#include <stdio.h>
#include <string.h>

#include "muse_ledger.h"

static int failures = 0;
#define CHECK(cond, name) do { \
    if (!(cond)) { printf("FAIL %s\n", name); failures++; } \
} while (0)

int main(void)
{
    char out[4096];

    size_t n = muse_ledger_format("hello lapis", "hello anduril",
                                  true, NULL, out, sizeof(out));
    CHECK(n > 0, "ledger: formats a delivered turn");
    CHECK(strstr(out, "heard: hello lapis") != NULL, "ledger: heard line");
    CHECK(strstr(out, "reply: hello anduril") != NULL, "ledger: reply line");
    CHECK(strstr(out, "outcome: delivered") != NULL, "ledger: outcome line");

    n = muse_ledger_format("x", NULL, false, "CAN'T REACH MUSE",
                           out, sizeof(out));
    CHECK(strstr(out, "outcome: failed (CAN'T REACH MUSE)") != NULL,
          "ledger: failed turn with note");

    n = muse_ledger_format("x", "y", true, "interrupted", out, sizeof(out));
    CHECK(strstr(out, "outcome: delivered (interrupted)") != NULL,
          "ledger: interrupted note");

    /* Truncation is honest: returns 0, never a half-line. */
    char small[16];
    CHECK(muse_ledger_format("hello", "world", true, NULL,
                             small, sizeof(small)) == 0,
          "ledger: truncation returns 0");

    /* The event flow doesn't crash without an SD card. */
    muse_ledger_heard("test heard");
    muse_ledger_reply("partial reply");
    muse_ledger_reply(" continued");
    muse_ledger_turn_end(true, NULL);

    if (failures == 0)
        printf("all ledger tests passed\n");
    else
        printf("%d FAILURES\n", failures);
    return failures != 0;
}
