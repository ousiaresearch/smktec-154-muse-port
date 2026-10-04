/*
 * muse_identity.c — NVS-backed instance identity.
 *
 * Draft for the Mac build (ESP-IDF v6.0.1). API use is standard NVS:
 * nvs_flash_init / nvs_open / nvs_get_* / nvs_set_* / nvs_commit.
 * VERIFY: first-boot wall-clock availability — birth_utc is 0 until SNTP
 * or the tunnel sets the clock; backfill on first sync (TODO in
 * muse_brain.c, Phase 2).
 */
#include "muse_identity.h"

#include <string.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_random.h"   /* esp_random() (IDF v6: no longer via esp_system.h) */
#include "esp_system.h"

static const char *TAG = "muse_identity";

int muse_identity_init(muse_identity_t *id)
{
    memset(id, 0, sizeof(*id));

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated or resized: erase and retry once. */
        ESP_LOGW(TAG, "nvs_flash_init failed (%d), erasing", err);
        err = nvs_flash_erase();
        if (err == ESP_OK)
            err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %d", err);
        return err;
    }

    nvs_handle_t h;
    err = nvs_open(MUSE_IDENTITY_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %d", err);
        return err;
    }

    size_t len = sizeof(id->name);
    if (nvs_get_str(h, "name", id->name, &len) != ESP_OK) {
        /* First boot: mint the identity. */
        strncpy(id->name, "Lapis", sizeof(id->name) - 1);
        strncpy(id->owner, "Anduril", sizeof(id->owner) - 1);
        id->birth_utc = 0; /* backfilled when the clock is set */
        id->boot_count = 0;
        id->generation = 1;
        id->seed = esp_random();   /* one soul, minted once */
        id->growth_stage = 0;
        id->care_days = 0;
        nvs_set_str(h, "name", id->name);
        nvs_set_str(h, "owner", id->owner);
        nvs_set_u32(h, "birth_utc", id->birth_utc);
        nvs_set_u32(h, "generation", id->generation);
        nvs_set_u32(h, "seed", id->seed);
        nvs_set_u32(h, "growth_stage", 0);
        nvs_set_u32(h, "care_days", 0);
        nvs_set_u32(h, "mastery", 0);
        float zero[MUSE_LEARN_DOMAINS] = { 0.5f, 0.5f };
        nvs_set_blob(h, "learn_base", zero, sizeof(zero));
        float mood0 = 0.0f;
        nvs_set_blob(h, "mood", &mood0, sizeof(mood0));
        float fam0[16] = { 0 };
        nvs_set_blob(h, "familiarity", fam0, sizeof(fam0));
        float bond0[2] = { 0.0f, 0.0f };
        nvs_set_blob(h, "bond", bond0, sizeof(bond0));
        ESP_LOGI(TAG, "minted identity: %s, generation %lu", id->name,
                 (unsigned long)id->generation);
    } else {
        len = sizeof(id->owner);
        nvs_get_str(h, "owner", id->owner, &len);
        nvs_get_u32(h, "birth_utc", &id->birth_utc);
        nvs_get_u32(h, "generation", &id->generation);
        nvs_get_u32(h, "boot_count", &id->boot_count);
        nvs_get_u32(h, "seed", &id->seed);
        nvs_get_u32(h, "growth_stage", &id->growth_stage);
        nvs_get_u32(h, "care_days", &id->care_days);
        nvs_get_u32(h, "mastery", &id->mastery);
        size_t blen = sizeof(id->learn_base);
        if (nvs_get_blob(h, "learn_base", id->learn_base, &blen) != ESP_OK) {
            id->learn_base[0] = id->learn_base[1] = 0.5f;
        }
        size_t mlen = sizeof(id->mood);
        if (nvs_get_blob(h, "mood", &id->mood, &mlen) != ESP_OK)
            id->mood = 0.0f;
        size_t flen = sizeof(id->familiarity);
        if (nvs_get_blob(h, "familiarity", id->familiarity, &flen) != ESP_OK)
            memset(id->familiarity, 0, sizeof(id->familiarity));
        float bond[2] = { 0.0f, 0.0f };
        size_t bondlen = sizeof(bond);
        if (nvs_get_blob(h, "bond", bond, &bondlen) == ESP_OK) {
            id->bond_lambda = bond[0];
            id->bond_beta = bond[1];
        }
    }

    id->boot_count++;
    nvs_set_u32(h, "boot_count", id->boot_count);
    nvs_commit(h);
    nvs_close(h);

    ESP_LOGI(TAG, "%s gen %lu boot %lu", id->name,
             (unsigned long)id->generation, (unsigned long)id->boot_count);
    return 0;
}

int muse_identity_factory_reset(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(MUSE_IDENTITY_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK)
        return err;
    uint32_t gen = 1, seed = 0;
    nvs_get_u32(h, "generation", &gen);
    nvs_get_u32(h, "seed", &seed);   /* read BEFORE the erase below */
    nvs_erase_all(h);
    /* The next init mints generation+1: the creature remembers dying.
     * The seed survives: same soul, new life. */
    nvs_set_u32(h, "generation", gen + 1);
    nvs_set_u32(h, "seed", seed ? seed : esp_random());
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGW(TAG, "identity wiped; next boot is generation %lu", (unsigned long)(gen + 1));
    return 0;
}

int muse_identity_save(const muse_identity_t *id)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(MUSE_IDENTITY_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK)
        return err;
    nvs_set_u32(h, "growth_stage", id->growth_stage);
    nvs_set_u32(h, "care_days", id->care_days);
    nvs_set_u32(h, "mastery", id->mastery);
    nvs_set_blob(h, "learn_base", id->learn_base, sizeof(id->learn_base));
    nvs_set_blob(h, "mood", &id->mood, sizeof(id->mood));
    nvs_set_blob(h, "familiarity", id->familiarity, sizeof(id->familiarity));
    float bond[2] = { id->bond_lambda, id->bond_beta };
    nvs_set_blob(h, "bond", bond, sizeof(bond));
    nvs_set_u32(h, "seed", id->seed);
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}
