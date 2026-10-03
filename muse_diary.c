/*
 * muse_diary.c — SD-card diary sink (FatFS).
 *
 * Draft for the Mac build (ESP-IDF v6.0.1).
 * VERIFY on hardware: TF slot SDMMC pins (slot-1 defaults assumed),
 * mount point, and that the card is FAT32.
 */
#include "muse_diary.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

static const char *TAG = "muse_diary";
static bool s_ready = false;

#define MOUNT_POINT "/sdcard"
#define DIARY_DIR   MOUNT_POINT "/lapis/diary"

static void diary_path(char *out, size_t n)
{
    time_t t = time(NULL);
    struct tm tm;
    /* Unset clock → one rolling file instead of a dated one. */
    if (t < 1700000000 || !localtime_r(&t, &tm)) {
        snprintf(out, n, DIARY_DIR "/undated.txt");
        return;
    }
    snprintf(out, n, DIARY_DIR "/%04d-%02d-%02d.txt",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

esp_err_t muse_diary_init(void)
{
    if (s_ready)
        return ESP_OK;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;   /* VERIFY: 1-bit vs 4-bit wiring on this board */
    /* VERIFY: slot gpio_cd / gpio_wp if the slot has detect pins. */

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,   /* never format the user's card */
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_card_t *card = NULL;
    esp_err_t err = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot,
                                            &mount_cfg, &card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "no SD card (%d) — diary disabled, boot continues", err);
        return err;
    }

    /* mkdir -p the diary tree. */
    mkdir(MOUNT_POINT "/lapis", 0755);
    if (mkdir(DIARY_DIR, 0755) != 0 && errno != EEXIST) {
        ESP_LOGW(TAG, "mkdir diary failed");
        return ESP_FAIL;
    }
    s_ready = true;
    ESP_LOGI(TAG, "diary ready at " DIARY_DIR);
    return ESP_OK;
}

void muse_diary_append(const char *line)
{
    if (!s_ready || !line)
        return;
    char path[128];
    diary_path(path, sizeof(path));
    FILE *f = fopen(path, "a");
    if (!f) {
        ESP_LOGW(TAG, "append failed");
        return;
    }
    time_t t = time(NULL);
    struct tm tm;
    if (t >= 1700000000 && localtime_r(&t, &tm))
        fprintf(f, "[%04d-%02d-%02d %02d:%02d] %s\n",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, line);
    else
        fprintf(f, "[clock-unset] %s\n", line);
    fclose(f);
}

bool muse_diary_ready(void)
{
    return s_ready;
}
