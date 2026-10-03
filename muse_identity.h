/*
 * muse_identity.h — the creature's persistent self.
 *
 * Personalization v1, item 1: instance identity stored in NVS, surviving
 * OTA and reboot. First boot mints the identity (name, owner, birth
 * timestamp); every boot after that increments the boot counter. A
 * factory reset wipes the namespace and bumps the generation, so a
 * reborn creature knows it has lived before.
 *
 * ESP-IDF draft — builds on the Mac against nvs_flash. Not host-testable
 * (NVS needs the IDF); verify with `build 154`.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Learning domains (Oudeyer): 0 = gesture confidence, 1 = voice-turn
 * success. Defined here because the baseline is persisted with identity. */
#define MUSE_LEARN_DOMAINS 2

#ifdef __cplusplus
extern "C" {
#endif

#define MUSE_IDENTITY_NVS_NS "lapis"
#define MUSE_IDENTITY_NAME_MAX 32

typedef struct {
    char name[MUSE_IDENTITY_NAME_MAX];   /* "Lapis" */
    char owner[MUSE_IDENTITY_NAME_MAX];  /* "Anduril" */
    uint32_t birth_utc;   /* first-boot unix time; 0 if clock was unset */
    uint32_t boot_count;  /* increments every init */
    uint32_t generation;  /* increments on factory reset */
    uint32_t seed;        /* minted once; survives factory reset */
    uint32_t growth_stage;/* 0..3 — see GROWTH.md */
    uint32_t care_days;   /* days with real interaction */
    uint32_t mastery;     /* learning-progress credits (Oudeyer) */
    float learn_base[MUSE_LEARN_DOMAINS]; /* persisted slow learning EMAs */
} muse_identity_t;

/*
 * Open NVS, load the identity or mint it on first boot, bump boot_count,
 * commit. Returns 0 on success, nonzero esp_err_t on failure. On failure
 * the struct is zeroed — callers must handle an unknown self honestly
 * (the face can show it; the gate treats missing identity as a default,
 * never as a reading).
 */
int muse_identity_init(muse_identity_t *id);

/* Wipe the namespace and start generation+1. The creature remembers dying. */
int muse_identity_factory_reset(void);

/* Persist growth fields (stage/care-days) after the dream pass. */
int muse_identity_save(const muse_identity_t *id);

#ifdef __cplusplus
}
#endif
