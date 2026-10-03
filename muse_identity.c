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
        nvs_set_str(h, "name", id->name);
        nvs_set_str(h, "owner", id->owner);
        nvs_set_u32(h, "birth_utc", id->birth_utc);
        nvs_set_u32(h, "generation", id->generation);
        ESP_LOGI(TAG, "minted identity: %s, generation %lu", id->name,
                 (unsigned long)id->generation);
    } else {
        len = sizeof(id->owner);
        nvs_get_str(h, "owner", id->owner, &len);
        nvs_get_u32(h, "birth_utc", &id->birth_utc);
        nvs_get_u32(h, "generation", &id->generation);
        nvs_get_u32(h, "boot_count", &id->boot_count);
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
    uint32_t gen = 1;
    nvs_get_u32(h, "generation", &gen);
    nvs_erase_all(h);
    /* The next init mints generation+1: the creature remembers dying. */
    nvs_set_u32(h, "generation", gen + 1);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGW(TAG, "identity wiped; next boot is generation %lu", (unsigned long)(gen + 1));
    return 0;
}
