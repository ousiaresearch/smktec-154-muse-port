/* Copyright (c) Meta Platforms, Inc. and affiliates.
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
 * muse_imu.c — QMI8658 driver wrapper + 50 Hz gesture detection engine.
 *
 * DRAFT. See muse_imu.h for the driver-source decision, wiring notes,
 * gesture mapping table, and the hardware VERIFY list.
 *
 * Design notes:
 *  - The engine is accelerometer-only. The gyro is configured but unused;
 *    it is reserved for future flick/rotation gestures.
 *  - RAM footprint: ~200 bytes of static state + one 4 KB task stack.
 *    No sample buffers, no PSRAM.
 *  - The callback fires on the IMU task: keep it short and non-blocking.
 */

#include "muse_imu.h"

/* Verified against waveshare/qmi8658 v1.0.0 (see components/qmi8658). */
#include "qmi8658.h"

#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "muse_imu";

/* ------------------------------------------------------------------ */
/* Tunables                                                            */
/* ------------------------------------------------------------------ */

#define MUSE_IMU_ADDR        0x6BU   /* VERIFY: 0x6B standard, 0x6A fallback probed */
#define MUSE_IMU_ADDR_ALT    0x6AU

#define POLL_PERIOD_MS       20      /* 50 Hz */
#define GRAV_ALPHA           0.05f   /* gravity low-pass; tau ~= 0.4 s at 50 Hz */

#define ENTER_COS            0.7071f /* enter a new orientation at 45 deg */
#define EXIT_COS             0.8660f /* leave the old one at 30 deg (hysteresis) */
#define ORIENT_GATE_G        0.40f   /* freeze orientation while energetic */

#define MOVE_THRESH_G        0.35f   /* dynamic-energy threshold: still -> moving */
#define STILL_THRESH_G       0.15f   /* dynamic-energy threshold: moving -> still */
#define MOVE_DEBOUNCE        3       /* consecutive samples (60 ms) */
#define STILL_DEBOUNCE       25      /* consecutive samples (500 ms) */

#define SHAKE_THRESH_G       1.0f    /* sustained dynamic energy for a shake */
#define SHAKE_TIME_MS        600
#define SHAKE_CALM_MS        800

#define TAP_MAG_G            1.8f    /* total-accel spike threshold */
#define TAP_END_G            1.35f   /* spike-over level */
#define TAP_MAX_SAMPLES      4       /* spike must end within 80 ms */
#define TAP_Z_MIN_G          0.9f    /* spike must be Z-dominant (screen tap) */
#define TAP_QUIET_SAMPLES    6       /* 120 ms quiet window after the spike */
#define TAP_QUIET_ABORT_G    1.5f    /* abort tap if it gets loud again */
#define DOUBLE_TAP_MS        400

#define PICKUP_STILL_MS      3000
#define STILL_30S_MS         30000
#define STILL_5MIN_MS        300000

#define IMU_TASK_STACK       4096
#define IMU_TASK_PRIO        5

/* Reference gravity directions in the body frame.
 * VERIFY on hardware: lay flat face-up -> expect az ~= +9.81;
 * tilt left edge down -> expect ax > 0. Flip signs below if mirrored. */
static const float s_ref[][3] = {
    [MUSE_IMU_TILT_UNKNOWN] = { 0.0f,  0.0f,  0.0f },
    [MUSE_IMU_FACE_UP]      = { 0.0f,  0.0f,  1.0f },
    [MUSE_IMU_FACE_DOWN]    = { 0.0f,  0.0f, -1.0f },
    [MUSE_IMU_TILT_LEFT]    = { 1.0f,  0.0f,  0.0f },
    [MUSE_IMU_TILT_RIGHT]   = {-1.0f,  0.0f,  0.0f },
    [MUSE_IMU_TILT_FWD]     = { 0.0f, -1.0f,  0.0f },
    [MUSE_IMU_TILT_BACK]    = { 0.0f,  1.0f,  0.0f },
};

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static bool s_inited;
static esp_err_t s_init_result;
static bool s_present;
static qmi8658_dev_t s_dev;

static TaskHandle_t s_task;
static muse_imu_cb_t s_cb;
static void *s_ctx;

/* Filter + engine state (all in m/s^2 unless noted). */
static bool s_seeded;
static float s_gx, s_gy, s_gz;      /* gravity vector, low-passed */
static muse_imu_tilt_t s_tilt;
static bool s_moving;
static int s_move_n, s_still_n;
static uint32_t s_still_since_ms;
static bool s_fired_30s, s_fired_5min;
static int s_shake_ms, s_calm_ms;
static bool s_shaking;

/* Tap detector FSM. */
typedef enum { TAP_IDLE, TAP_SPIKE, TAP_QUIET } tap_state_t;
static tap_state_t s_tap;
static int s_tap_n;
static uint32_t s_prev_tap_ms;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void emit(muse_imu_gesture_t g)
{
    if (s_cb == NULL) {
        return;
    }
    const muse_imu_event_t ev = {
        .gesture = g,
        .tilt = s_tilt,
        .time_ms = now_ms(),
    };
    s_cb(&ev, s_ctx);
}

/* ------------------------------------------------------------------ */
/* Gesture engine: one 50 Hz sample (ax/ay/az in m/s^2)                */
/* ------------------------------------------------------------------ */

static void engine_sample(float ax, float ay, float az, uint32_t now)
{
    const float G = 9.81f;

    /* Seed the gravity filter on the first sample so orientation is valid
     * immediately instead of converging over ~1 s. */
    if (!s_seeded) {
        s_gx = ax;
        s_gy = ay;
        s_gz = az;
        s_seeded = true;
    }

    /* Treat boot as the start of a still period so the stillness timers
     * work even if the device never moves after power-on. */
    if (s_still_since_ms == 0) {
        s_still_since_ms = now;
    }

    /* Gravity (low-pass) and dynamic components. */
    s_gx += GRAV_ALPHA * (ax - s_gx);
    s_gy += GRAV_ALPHA * (ay - s_gy);
    s_gz += GRAV_ALPHA * (az - s_gz);
    const float dx = ax - s_gx;
    const float dy = ay - s_gy;
    const float dz = az - s_gz;

    const float energy = sqrtf(dx * dx + dy * dy + dz * dz) / G; /* g */
    const float mag = sqrtf(ax * ax + ay * ay + az * az) / G;    /* g */
    const float dyn_z = dz / G;                                  /* g */

    /* ---- still / moving with hysteresis + debounce ---- */
    if (!s_moving) {
        if (energy > MOVE_THRESH_G) {
            if (++s_move_n >= MOVE_DEBOUNCE) {
                s_moving = true;
                s_move_n = 0;
                s_still_n = 0;
                if (now - s_still_since_ms >= PICKUP_STILL_MS && s_still_since_ms != 0) {
                    emit(MUSE_IMU_PICKUP);
                }
                s_fired_30s = false;
                s_fired_5min = false;
                s_shake_ms = 0;
                emit(MUSE_IMU_MOVING);
            }
        } else {
            s_move_n = 0;
        }
    } else {
        if (energy < STILL_THRESH_G) {
            if (++s_still_n >= STILL_DEBOUNCE) {
                s_moving = false;
                s_still_n = 0;
                s_move_n = 0;
                s_still_since_ms = now;
                emit(MUSE_IMU_STILL);
            }
        } else {
            s_still_n = 0;
        }
    }

    /* ---- stillness timers (fire once per still period) ---- */
    if (!s_moving && s_still_since_ms != 0) {
        const uint32_t still_for = now - s_still_since_ms;
        if (!s_fired_30s && still_for >= STILL_30S_MS) {
            s_fired_30s = true;
            emit(MUSE_IMU_STILL_30S);
        }
        if (!s_fired_5min && still_for >= STILL_5MIN_MS) {
            s_fired_5min = true;
            emit(MUSE_IMU_STILL_5MIN);
        }
    }

    /* ---- shake ---- */
    if (energy > SHAKE_THRESH_G) {
        s_shake_ms += POLL_PERIOD_MS;
    } else {
        s_shake_ms = 0;
    }
    if (!s_shaking && s_shake_ms >= SHAKE_TIME_MS) {
        s_shaking = true;
        s_calm_ms = 0;
        emit(MUSE_IMU_SHAKE_START);
    }
    if (s_shaking) {
        if (energy < STILL_THRESH_G) {
            s_calm_ms += POLL_PERIOD_MS;
        } else {
            s_calm_ms = 0;
        }
        if (s_calm_ms >= SHAKE_CALM_MS) {
            s_shaking = false;
            s_calm_ms = 0;
            s_shake_ms = 0;
            emit(MUSE_IMU_SHAKE_END);
        }
    }

    /* ---- orientation (frozen while energetic to avoid chatter) ---- */
    if (energy < ORIENT_GATE_G) {
        const float nx = s_gx / G;
        const float ny = s_gy / G;
        const float nz = s_gz / G;
        const float cx = fabsf(nx);
        const float cy = fabsf(ny);
        const float cz = fabsf(nz);

        muse_imu_tilt_t prop = MUSE_IMU_TILT_UNKNOWN;
        float c = 0.0f;
        if (cx >= cy && cx >= cz) {
            prop = (nx > 0.0f) ? MUSE_IMU_TILT_LEFT : MUSE_IMU_TILT_RIGHT;
            c = cx;
        } else if (cy >= cx && cy >= cz) {
            prop = (ny > 0.0f) ? MUSE_IMU_TILT_BACK : MUSE_IMU_TILT_FWD;
            c = cy;
        } else {
            prop = (nz > 0.0f) ? MUSE_IMU_FACE_UP : MUSE_IMU_FACE_DOWN;
            c = cz;
        }

        /* Hysteresis: adopt the new orientation only past 45 deg, and only
         * once the old one has drifted past 30 deg from its ideal. */
        float conf = -1.0f;
        if (s_tilt != MUSE_IMU_TILT_UNKNOWN) {
            conf = nx * s_ref[s_tilt][0] + ny * s_ref[s_tilt][1] + nz * s_ref[s_tilt][2];
        }
        if (c > ENTER_COS && prop != s_tilt && conf < EXIT_COS) {
            s_tilt = prop;
            emit(MUSE_IMU_ORIENTATION);
        }
    }

    /* ---- tap: Z-dominant spike + quiet window ----
     * Speaker vibration and button presses are the false-trigger sources;
     * the Z-dominance requirement plus the mandatory quiet window are the
     * mitigation. Taps are suppressed while a shake is in progress. */
    if (s_shaking || s_shake_ms > 0) {
        s_tap = TAP_IDLE;
    } else {
        switch (s_tap) {
        case TAP_IDLE:
            if (mag > TAP_MAG_G && fabsf(dyn_z) > TAP_Z_MIN_G) {
                s_tap = TAP_SPIKE;
                s_tap_n = 0;
            }
            break;
        case TAP_SPIKE:
            s_tap_n++;
            if (mag < TAP_END_G) {
                s_tap = (s_tap_n <= TAP_MAX_SAMPLES) ? TAP_QUIET : TAP_IDLE;
                s_tap_n = 0;
            } else if (s_tap_n > TAP_MAX_SAMPLES) {
                s_tap = TAP_IDLE; /* sustained motion, not a tap */
            }
            break;
        case TAP_QUIET:
            s_tap_n++;
            if (mag > TAP_QUIET_ABORT_G) {
                s_tap = TAP_IDLE;
            } else if (s_tap_n >= TAP_QUIET_SAMPLES) {
                s_tap = TAP_IDLE;
                if (s_prev_tap_ms != 0 && now - s_prev_tap_ms < DOUBLE_TAP_MS) {
                    s_prev_tap_ms = 0;
                    emit(MUSE_IMU_DOUBLE_TAP);
                } else {
                    s_prev_tap_ms = now;
                    emit(MUSE_IMU_TAP);
                }
            }
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* FreeRTOS task                                                       */
/* ------------------------------------------------------------------ */

static void imu_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        bool ready = false;
        if (qmi8658_is_data_ready(&s_dev, &ready) == ESP_OK && ready) {
            qmi8658_data_t d;
            if (qmi8658_read_sensor_data(&s_dev, &d) == ESP_OK) {
                /* Driver configured for m/s^2 (see muse_imu_init). */
                engine_sample(d.accelX, d.accelY, d.accelZ, now_ms());
            }
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(POLL_PERIOD_MS));
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

esp_err_t muse_imu_init(i2c_master_bus_handle_t bus)
{
    if (s_inited) {
        return s_init_result; /* idempotent */
    }
    s_inited = true;
    s_init_result = ESP_OK;

    memset(&s_dev, 0, sizeof(s_dev));

    if (bus == NULL) {
        ESP_LOGW(TAG, "no I2C bus handle; IMU disabled");
        return s_init_result; /* graceful: absent, boot continues */
    }

    /* Probe the standard address, then the alternate. */
    esp_err_t e = qmi8658_init(&s_dev, bus, MUSE_IMU_ADDR);
    if (e != ESP_OK) {
        ESP_LOGI(TAG, "no QMI8658 at 0x%02X, trying 0x%02X", MUSE_IMU_ADDR, MUSE_IMU_ADDR_ALT);
        e = qmi8658_init(&s_dev, bus, MUSE_IMU_ADDR_ALT);
    }
    if (e != ESP_OK) {
        /* qmi8658_init() does a WHO_AM_I check and fails cleanly when the
         * chip is absent; we never touch the bus on this path. */
        ESP_LOGW(TAG, "QMI8658 not found; IMU disabled (%s)", esp_err_to_name(e));
        return s_init_result; /* graceful: absent, boot continues */
    }

    e = qmi8658_set_accel_range(&s_dev, QMI8658_ACCEL_RANGE_8G);
    if (e == ESP_OK) {
        e = qmi8658_set_accel_odr(&s_dev, QMI8658_ACCEL_ODR_1000HZ);
    }
    if (e == ESP_OK) {
        e = qmi8658_set_gyro_range(&s_dev, QMI8658_GYRO_RANGE_512DPS);
    }
    if (e == ESP_OK) {
        e = qmi8658_set_gyro_odr(&s_dev, QMI8658_GYRO_ODR_1000HZ);
    }
    /* Unit setters return void in qmi8658 v1.0.0. */
    qmi8658_set_accel_unit_mps2(&s_dev, true);
    qmi8658_set_gyro_unit_rads(&s_dev, true);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "QMI8658 configure failed; IMU disabled (%s)", esp_err_to_name(e));
        return s_init_result;
    }
    /* TODO: drop the accel ODR once the driver's full ODR enum is confirmed;
     * ~112-224 Hz is plenty for a 50 Hz poll loop and saves battery. */

    s_present = true;
    ESP_LOGI(TAG, "QMI8658 ready (accel 8 g, gyro 512 dps, SI units)");
    return s_init_result;
}

void muse_imu_deinit(void)
{
    muse_imu_stop();
    s_present = false;
    s_inited = false;
    memset(&s_dev, 0, sizeof(s_dev));
}

esp_err_t muse_imu_start(void)
{
    if (!s_present) {
        return ESP_OK; /* no-op when the IMU is absent */
    }
    if (s_task != NULL) {
        return ESP_OK; /* already running */
    }
    if (xTaskCreate(imu_task, "muse_imu", IMU_TASK_STACK, NULL, IMU_TASK_PRIO, &s_task) != pdPASS) {
        ESP_LOGE(TAG, "task create failed");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void muse_imu_stop(void)
{
    if (s_task != NULL) {
        vTaskDelete(s_task);
        s_task = NULL;
    }
}

void muse_imu_set_callback(muse_imu_cb_t cb, void *ctx)
{
    s_cb = cb;
    s_ctx = ctx;
}

bool muse_imu_present(void)
{
    return s_present;
}

muse_imu_tilt_t muse_imu_tilt(void)
{
    return s_tilt;
}

bool muse_imu_moving(void)
{
    return s_moving;
}
