/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * SMKTelec ESP32-S3 1.54" Touch LCD (clone of Waveshare ESP32-S3-Touch-LCD-1.54):
 * ESP32-S3R8 (8 MB octal PSRAM), 16 MB flash, 1.54" 240x240 ST7789 LCD with
 * CST816S capacitive touch, ES7210 dual-mic array + ES8311 codec + speaker,
 * 3 buttons (PWR, PLUS, BOOT), battery circuit, microSD.
 *
 * DRAFT — written before hardware arrival. Pins follow xiaozhi-esp32's board
 * definition for waveshare/esp32-s3-touch-lcd-1.54 (config.h) and the
 * Waveshare spec sheet. VERIFY items are marked inline; do not flash blind.
 *
 * Structure follows board_aipi.c (drives esp_lcd + codec + GPIOs directly).
 */
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/usb_serial_jtag.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_codec_dev_defaults.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch_cst816s.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "muse_audio.h"
#include "muse_board.h"
#include "muse_brain.h"
#include "muse_diary.h"
#include "muse_identity.h"
#include "muse_mem.h"
#include "muse_state.h"
#include "muse_turn_gate.h"

#include <string.h>
#include <time.h>

static const char *TAG = "board";

/* Display: ST7789, 1.54" 240x240, 4-wire SPI mode 3. */
#define LCD_RES 240
#define LCD_HOST SPI3_HOST
#define LCD_SCLK GPIO_NUM_38
#define LCD_MOSI GPIO_NUM_39
#define LCD_CS GPIO_NUM_21
#define LCD_DC GPIO_NUM_45
#define LCD_RST GPIO_NUM_40
#define LCD_BL GPIO_NUM_46
#define DRAW_BUF_LINES 32

/* Touch: CST816S on the shared I2C bus. */
#define TP_INT GPIO_NUM_48
#define TP_RST GPIO_NUM_47

/* Shared I2C bus: audio codecs + touch controller. */
#define I2C_SDA GPIO_NUM_42
#define I2C_SCL GPIO_NUM_41

/* Audio: ES7210 dual mic (in) + ES8311 codec (out), NS4150B amp. */
#define I2S_MCLK GPIO_NUM_8
#define I2S_BCLK GPIO_NUM_9
#define I2S_WS GPIO_NUM_10
#define I2S_DIN GPIO_NUM_11    /* mic data into the S3 */
#define I2S_DOUT GPIO_NUM_12   /* speaker data out of the S3 */
#define PA_EN GPIO_NUM_7       /* speaker amp enable, active high */

/* Buttons. */
#define TALK_GPIO GPIO_NUM_5   /* PWR button */
#define AUX_GPIO GPIO_NUM_0    /* BOOT button */
#define PLUS_GPIO GPIO_NUM_4   /* PLUS custom button — TODO: assign (volume? pet?);
                                * muse_board_t only exposes talk+aux, so this
                                * needs a wiring decision on bring-up day. */

/* Battery circuit. */
#define BATT_EN GPIO_NUM_2     /* drive high to enable battery path */
#define BATT_ADC_CH ADC_CHANNEL_0  /* GPIO1; VERIFY channel mapping on S3 */
#define CHARGE_GPIO GPIO_NUM_3 /* high while charging */

static i2c_master_bus_handle_t s_i2c;
static esp_lcd_panel_handle_t s_panel;
static muse_gpio_button_t s_talk, s_aux;
static adc_oneshot_unit_handle_t s_adc;

/* The creature's self and nervous system (see INTEGRATION.md §10). */
static muse_identity_t s_identity;
static muse_brain_state_t s_brain;
static bool s_brain_ready = false;

/* SNTP sync notice: the diary timestamps and quiet hours go live here. */
static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    ESP_LOGI(TAG, "SNTP synced");
}

static esp_err_t init(void)
{
    /* Enable the battery path (xiaozhi PowerON()). */
    gpio_config_t ben = {
        .pin_bit_mask = 1ULL << BATT_EN,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&ben), TAG, "batt en");
    ESP_RETURN_ON_ERROR(gpio_set_level(BATT_EN, 1), TAG, "batt en on");

    const i2c_master_bus_config_t i2c_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_cfg, &s_i2c), TAG, "i2c");
    ESP_RETURN_ON_ERROR(muse_gpio_button_init(&s_talk, TALK_GPIO), TAG, "talk button");
    ESP_RETURN_ON_ERROR(muse_gpio_button_init(&s_aux, AUX_GPIO), TAG, "aux button");

    gpio_config_t chg = {
        .pin_bit_mask = 1ULL << CHARGE_GPIO,
        .mode = GPIO_MODE_INPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&chg), TAG, "charge pin");
    const adc_oneshot_unit_init_cfg_t adc_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&adc_cfg, &s_adc), TAG, "adc");
    const adc_oneshot_chan_cfg_t ch_cfg = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    /* TODO: calibrate the divider levels on hardware (see read_power). */
    esp_err_t adc_err = adc_oneshot_config_channel(s_adc, BATT_ADC_CH, &ch_cfg);

    /* SNTP: wall-clock time for diary timestamps, quiet hours, and the
     * SCN phase. Best-effort — runs whenever Wi-Fi is up, harmless
     * without it. VERIFY on IDF v6.0.1: esp_sntp API names. */
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);  /* TODO: settings page */
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(sntp_sync_cb);
    esp_sntp_init();

    /* The creature wakes up as someone: identity first (NVS mint or load),
     * then the diary (best-effort SD mount), then the brain with the diary
     * as its log sink. None of this fails the boot. */
    if (muse_identity_init(&s_identity) != 0)
        ESP_LOGW(TAG, "identity unavailable — the self is unknown this boot");
    muse_diary_init();
    muse_brain_init(&s_brain, muse_diary_append);
    muse_brain_learning_restore(&s_brain, &s_identity);  /* capability baseline */
    muse_turn_gate_attach(&s_brain, &s_identity);       /* PTT middleware */
    s_brain_ready = true;
    return adc_err;
}

static lv_display_t *display_start(lv_indev_t **touch)
{
    const ledc_timer_config_t bl_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    const ledc_channel_config_t bl_ch = {
        .gpio_num = LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    if (ledc_timer_config(&bl_timer) != ESP_OK || ledc_channel_config(&bl_ch) != ESP_OK) {
        return NULL;
    }

    const spi_bus_config_t bus = {
        .sclk_io_num = LCD_SCLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_RES * DRAW_BUF_LINES * 2,
    };
    if (spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK) {
        return NULL;
    }
    esp_lcd_panel_io_handle_t io;
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_CS,
        .dc_gpio_num = LCD_DC,
        .spi_mode = 3,
        .pclk_hz = 40 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    if (esp_lcd_new_panel_io_spi(LCD_HOST, &io_cfg, &io) != ESP_OK) {
        return NULL;
    }
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    if (esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel) != ESP_OK) {
        return NULL;
    }
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    /* VERIFY: xiaozhi sets invert=true for this panel; if colors look wrong, flip. */
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_disp_on_off(s_panel, true);

    /* Touch: CST816S on the shared I2C bus. Created here, registered with
     * the LVGL adapter after the display exists (see below). */
    esp_lcd_touch_handle_t tp = NULL;
    {
        esp_lcd_panel_io_handle_t tp_io;
        /* Canonical per-device IO config from esp_lcd_touch_cst816s
         * (ESP_LCD_TOUCH_IO_I2C_CST816S_ADDRESS = 0x15); the chip does
         * 400 kHz fine and the shared bus stays usable for the codecs. */
        esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
        tp_io_cfg.scl_speed_hz = 400000;
        if (esp_lcd_new_panel_io_i2c(s_i2c, &tp_io_cfg, &tp_io) == ESP_OK) {
            const esp_lcd_touch_config_t tp_cfg = {
                .x_max = LCD_RES,
                .y_max = LCD_RES,
                .rst_gpio_num = TP_RST,
                .int_gpio_num = TP_INT,
                .levels = {
                    .reset = 0,
                    .interrupt = 0,
                },
                .flags = {
                    .swap_xy = 0,
                    .mirror_x = 0,
                    .mirror_y = 0,
                },
            };
            /* Constructor + config verified against
             * espressif/esp_lcd_touch_cst816s ^1.0.0
             * (esp_lcd_touch_new_i2c_cst816s(io, &tp_cfg, &tp); the
             * interrupt_callback field is optional and left unset — the
             * LVGL adapter drives reads after registration below). */
            if (esp_lcd_touch_new_i2c_cst816s(tp_io, &tp_cfg, &tp) != ESP_OK) {
                ESP_LOGE(TAG, "cst816s init failed");
                tp = NULL;
            }
        } else {
            ESP_LOGE(TAG, "touch io failed");
        }
    }

    esp_lv_adapter_config_t adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_cfg.task_core_id = MUSE_UI_CORE;
    adapter_cfg.task_priority = MUSE_UI_PRIORITY;
    if (esp_lv_adapter_init(&adapter_cfg) != ESP_OK) {
        return NULL;
    }
    const esp_lv_adapter_display_config_t disp_cfg = {
        .panel = s_panel,
        .panel_io = io,
        .profile = {
            .interface = ESP_LV_ADAPTER_PANEL_IF_OTHER,
            .rotation = ESP_LV_ADAPTER_ROTATE_0,
            .hor_res = LCD_RES,
            .ver_res = LCD_RES,
            .buffer_height = DRAW_BUF_LINES,
            .use_psram = false,
            .require_double_buffer = true,
        },
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
    };
    lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    if (!disp) {
        return NULL;
    }
    /* Register touch now that the display exists (watcher pattern). */
    if (tp) {
        esp_lv_adapter_touch_config_t lv_tp_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, tp);
        *touch = esp_lv_adapter_register_touch(&lv_tp_cfg);
        if (!*touch) {
            ESP_LOGE(TAG, "touch register failed");
        }
    } else {
        *touch = NULL;
    }
    if (esp_lv_adapter_start() != ESP_OK) {
        return NULL;
    }
    return disp;
}

static bool display_lock(int timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

static void set_brightness(int pct)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pct * 1023 / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void panel_sleep(bool sleep)
{
    esp_lcd_panel_disp_sleep(s_panel, sleep);   /* SLPIN/SLPOUT; GRAM is kept */
}

static void display_pause(bool pause)
{
    if (pause) {
        esp_lv_adapter_pause(-1);
    } else {
        esp_lv_adapter_resume();
    }
    /* TODO: consider sleeping the CST816S while paused (see watcher). */
}

/*
 * ES8311 (speaker out) + ES7210 (dual-mic in) on one duplex I2S bus.
 * Audio APIs verified against esp_codec_dev ~1.5 (1.5.11): es8311_codec_new /
 * es8311_codec_cfg_t { ctrl_if, gpio_if, codec_mode, pa_pin, use_mclk, ... },
 * es7210_codec_new / es7210_codec_cfg_t { ctrl_if, master_mode,
 * mic_selected, mclk_src, mclk_div }, audio_codec_new_i2s_data /
 * audio_codec_new_i2c_ctrl / audio_codec_new_gpio, esp_codec_dev_new.
 * I2C addrs are the 8-bit forms the 1.x ctrl interface expects
 * (ES8311 0x30 -> 0x18, ES7210 0x80 -> 0x40 on the wire).
 */
static esp_err_t audio_init(esp_codec_dev_handle_t *spk, esp_codec_dev_handle_t *mic)
{
    i2s_chan_handle_t tx, rx;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &tx, &rx), TAG, "i2s channel");
    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MUSE_AUDIO_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK,
            .bclk = I2S_BCLK,
            .ws = I2S_WS,
            .dout = I2S_DOUT,
            .din = I2S_DIN,
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx, &std_cfg), TAG, "i2s tx");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx, &std_cfg), TAG, "i2s rx");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx), TAG, "i2s tx on");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx), TAG, "i2s rx on");

    audio_codec_i2s_cfg_t i2s_cfg = { .port = I2S_NUM_0, .rx_handle = rx, .tx_handle = tx };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    ESP_RETURN_ON_FALSE(data_if && gpio_if, ESP_ERR_NO_MEM, TAG, "codec interfaces");

    /* Speaker path: ES8311. */
    audio_codec_i2c_cfg_t es8311_i2c = {
        .port = I2C_NUM_0, .addr = ES8311_CODEC_DEFAULT_ADDR, .bus_handle = s_i2c
    };
    const audio_codec_ctrl_if_t *es8311_ctrl = audio_codec_new_i2c_ctrl(&es8311_i2c);
    ESP_RETURN_ON_FALSE(es8311_ctrl, ESP_FAIL, TAG, "es8311 ctrl");
    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = es8311_ctrl,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = PA_EN,
        .use_mclk = true,
    };
    const audio_codec_if_t *spk_codec = es8311_codec_new(&es8311_cfg);
    ESP_RETURN_ON_FALSE(spk_codec, ESP_FAIL, TAG, "ES8311 not responding");

    /* Mic path: ES7210 dual-mic array. */
    audio_codec_i2c_cfg_t es7210_i2c = {
        .port = I2C_NUM_0, .addr = ES7210_CODEC_DEFAULT_ADDR, .bus_handle = s_i2c
    };
    const audio_codec_ctrl_if_t *es7210_ctrl = audio_codec_new_i2c_ctrl(&es7210_i2c);
    ESP_RETURN_ON_FALSE(es7210_ctrl, ESP_FAIL, TAG, "es7210 ctrl");
    /* Verified against esp_codec_dev 1.5.x
     * (device/include/es7210_adc.h, device/es7210/es7210.c): the 1.x config
     * has NO gpio_if member — it is { ctrl_if, master_mode, mic_selected,
     * mclk_src, mclk_div }. mic_selected is required: 0 selects no mics and
     * the driver's open() fails ("Microphone selection error"). With <3
     * mics the driver skips TDM and uses plain I2S: MIC1 = left slot,
     * MIC2 = right slot, matching the S3's Philips stereo RX below.
     * muse_audio opens the mic dev at 2 channels and mixes both (mic_slot=-1).
     * VERIFY on hardware: which physical mic lands on which slot. */
    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = es7210_ctrl,
        .master_mode = false,   /* S3 is the I2S master (I2S_ROLE_MASTER) */
        .mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2,
    };
    const audio_codec_if_t *mic_codec = es7210_codec_new(&es7210_cfg);
    ESP_RETURN_ON_FALSE(mic_codec, ESP_FAIL, TAG, "ES7210 not responding");

    esp_codec_dev_cfg_t out_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT, .codec_if = spk_codec, .data_if = data_if
    };
    esp_codec_dev_cfg_t in_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN, .codec_if = mic_codec, .data_if = data_if
    };
    *spk = esp_codec_dev_new(&out_cfg);
    *mic = esp_codec_dev_new(&in_cfg);
    return *spk && *mic ? ESP_OK : ESP_FAIL;
}

static void set_mic_gain(esp_codec_dev_handle_t mic, int db)
{
    /* ES7210 PGA steps are 3 dB; snap so the UI shows what's applied. */
    db = (db / 3) * 3;
    /* esp_codec_dev rounds 33 dB down to 30; the next real step up is 34.5. */
    esp_codec_dev_set_in_gain(mic, db == 33 ? 34.5f : (float)db);
}

static unsigned poll_buttons(void)
{
    return muse_gpio_button_poll(&s_talk) | muse_gpio_button_poll(&s_aux) << 2;
}

static void wait_buttons(int timeout_ms)
{
    muse_gpio_buttons_wait((muse_gpio_button_t *const[]){ &s_talk, &s_aux }, 2, timeout_ms);
}

/* Personalization wiring (INTEGRATION.md §10). Non-static: the SLEEPY mode
 * (§9) and the future gate middleware call these. */

/* Brain accessor for the gate middleware (voice turn loop, SDK core). */
muse_brain_state_t *smktec_brain(void)
{
    return s_brain_ready ? &s_brain : NULL;
}

/* Identity accessor for the vitals page and the context hook. */
muse_identity_t *smktec_identity(void)
{
    return s_brain_ready ? &s_identity : NULL;
}

/* Called on SLEEPY entry: the muse dreams (diary via the log sink).
 * Growth is evaluated here; the identity is persisted afterwards. */
void smktec_note_sleepy(bool entering)
{
    if (entering && s_brain_ready) {
        muse_brain_consolidate(&s_brain, &s_identity);
        muse_identity_save(&s_identity);
    }
}

/* Battery voltage on GPIO1 through the board's divider; levels are
 * PLACEHOLDERS — calibrate against a multimeter on hardware. */
static esp_err_t read_power(muse_power_t *out)
{
    int raw = 0;
    ESP_RETURN_ON_ERROR(adc_oneshot_read(s_adc, BATT_ADC_CH, &raw), TAG, "adc read");
    bool charging = gpio_get_level(CHARGE_GPIO) == 1;
    out->charging = charging;
    out->usb = usb_serial_jtag_is_connected();
    out->battery_pct = charging ? 100 : 50;   /* TODO: map raw to pct */
    out->battery_mv = 0;                       /* TODO: uncalibrated */
    (void)raw;

    /* Brain tick — muse_input polls read_power every 2s awake / 10s paused.
     * Battery only feeds the brain once the divider is calibrated; until
     * then the brain honestly reports stale energy (neutral defaults). */
    if (s_brain_ready) {
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (out->battery_mv > 0)
            muse_brain_feed_battery(&s_brain, out->battery_mv / 1000.0f,
                                    charging, now_ms);
        /* Wall-clock circadian: once SNTP has set the clock, quiet hours
         * (22:00–07:00 local) and the SCN phase come from real time. */
        time_t t = time(NULL);
        if (t >= 1700000000) {
            struct tm tm;
            if (localtime_r(&t, &tm)) {
                bool quiet = tm.tm_hour >= 22 || tm.tm_hour < 7;
                float phase = (tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec)
                              / 86400.0f;
                muse_brain_set_quiet_hours(&s_brain, quiet, phase, now_ms);
            }
        }
        muse_brain_tick(&s_brain, now_ms, muse_state_asleep());
    }
    return ESP_OK;
}

static esp_err_t power_off(void)
{
    /* No power latch on this board: just sleep the panel and deep-sleep.
     * VERIFY: wake source — PWR button (GPIO5) ext0 wakeup. */
    set_brightness(0);
    esp_lcd_panel_disp_on_off(s_panel, false);
    esp_sleep_enable_ext0_wakeup(TALK_GPIO, 0);
    esp_deep_sleep_start();
    return ESP_FAIL;
}

static const muse_board_t s_board = {
    .name = "SMKTelec ESP32-S3-Touch-LCD-1.54",
    .width = LCD_RES,
    .height = LCD_RES,
    .round = false,
    .touch = true,
    .diagonal_in = 1.54f,
    .talk_button = "left side",
    .aux_button = "left side",
    /* VERIFY physical layout on arrival; hints place captions near buttons. */
    .talk_hint = { LV_ALIGN_TOP_LEFT, 10, 30 },
    .aux_hint = { LV_ALIGN_BOTTOM_LEFT, 10, -30 },
    .frame_ms = 40,
    .init = init,
    .display_start = display_start,
    .display_lock = display_lock,
    .display_unlock = esp_lv_adapter_unlock,
    .set_brightness = set_brightness,
    .panel_sleep = panel_sleep,
    .display_pause = display_pause,
    .audio_init = audio_init,
    .mic_slot = -1,             /* dual-mic array: mix both slots */
    .set_mic_gain = set_mic_gain,
    .poll_buttons = poll_buttons,
    .wait_buttons = wait_buttons,
    .read_power = read_power,
    .power_off = power_off,
};

/* Home Link's app_main starts Muse with this board (main/main.c). */
const muse_board_t *muse_board_get(void)
{
    return &s_board;
}
