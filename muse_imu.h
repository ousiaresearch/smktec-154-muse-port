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
 * muse_imu.h — QMI8658 6-axis IMU driver wrapper + gesture detection engine.
 *
 * DRAFT for the SMKTelec ESP32-S3 1.54" Touch LCD port
 * (Waveshare ESP32-S3-Touch-LCD-1.54 clone). Not yet hardware-tested.
 *
 * ---------------------------------------------------------------------------
 * DRIVER-SOURCE DECISION
 * ---------------------------------------------------------------------------
 * Managed component: `waveshare/qmi8658==1.0.0` from the Espressif component
 * registry (`idf.py add-dependency "waveshare/qmi8658==1.0.0"`).
 *
 * Why this one:
 *  - C API (matches this firmware's C codebase).
 *  - Written by Waveshare for Waveshare boards; this board is a Waveshare
 *    ESP32-S3-Touch-LCD-1.54 clone, so register defaults and quirks match.
 *  - `qmi8658_init(&dev, bus_handle, addr)` attaches to an EXISTING
 *    i2c_master_bus_handle_t — it does not create or reconfigure the bus,
 *    which is required here because the bus is shared with the CST816S touch
 *    controller and the ES7210/ES8311 audio codecs (see below).
 *  - Built-in unit conversion (m/s^2, rad/s) via
 *    qmi8658_set_accel_unit_mps2() / qmi8658_set_gyro_unit_rads().
 *
 * Rejected alternatives:
 *  - esp-iot-solution's qmi8658 driver: NOT published to the component
 *    registry (only consumable as a git submodule); no clean
 *    idf_component.yml path for ESP-IDF v6.
 *  - espp/qmi8658 (v1.3.3): C++ (espp framework); wrong language for this
 *    component tree.
 *
 * Add to esp32/components/muse/idf_component.yml (draft snippet):
 *
 *   waveshare/qmi8658:
 *     version: "==1.0.0"
 *     rules:
 *       - if: "$CONFIG{MUSE_BOARD_ID} == \"smktec_s3_touch_lcd_154\""
 *
 * VERIFY at build time against the component's installed headers:
 *  - exact device-handle type name (`qmi8658_handle_t` assumed below),
 *  - `#include "qmi8658.h"` include path,
 *  - accel/gyro ODR enum value names (only ODR_1000HZ confirmed from docs),
 *  - that qmi8658_init() performs a WHO_AM_I check and returns an error when
 *    the chip is absent (graceful-disable depends on this),
 *  - that qmi8658_init() does NOT touch the bus configuration.
 *
 * Fallback register map (QMI8658), kept here so a minimal hand-written
 * driver can replace the managed one without re-research:
 *  WHO_AM_I 0x00 (expect 0x05) | RESET 0x60 (write 0xB0) | CTRL1 0x02 (accel)
 *  CTRL2 0x03 (gyro) | CTRL3 0x04 | CTRL5 0x06 | CTRL7 0x08 (sensor enable)
 *  STATUS0 0x2D (data-ready) | TEMP 0x33-0x34 | ACCEL 0x35-0x3A | GYRO 0x3B-0x40
 *
 * ---------------------------------------------------------------------------
 * WIRING (integration notes — no code changes made here)
 * ---------------------------------------------------------------------------
 * The shared I2C bus (SDA=GPIO42, SCL=GPIO41, 400 kHz) is created by the board
 * driver (board_smktec_s3_touch_lcd_154.c, static s_i2c). This module must be
 * handed that handle AFTER the bus exists; it never calls i2c_new_master_bus
 * itself. Suggested call site, right after the touch controller is up:
 *
 *     muse_imu_init(s_i2c);          // probe + configure; safe if IMU absent
 *     muse_imu_set_callback(imu_gesture_cb, NULL);
 *     muse_imu_start();              // 50 Hz task; no-op if IMU absent
 *
 * The gesture callback runs on the IMU task. Keep it short and non-blocking.
 * muse_state_set_mode() / muse_state_make_happy() are safe to call from it
 * (word-sized stores / esp_timer reads per components/muse/muse_state.h).
 *
 * ---------------------------------------------------------------------------
 * SUGGESTED GESTURE -> FIRMWARE MAPPING (for main/app.c)
 * ---------------------------------------------------------------------------
 *  MUSE_IMU_FACE_DOWN            muse_state_set_mode(MUSE_MODE_OFF)     // sleep face
 *  MUSE_IMU_FACE_UP (from down)  muse_state_set_mode(MUSE_MODE_IDLE)    // wake
 *  MUSE_IMU_TILT_LEFT/RIGHT      muse_pixel_set_facing(-1/+1)           // avatar look dir
 *                                (planned renderer hook; not yet implemented)
 *  MUSE_IMU_TILT_FWD/BACK        reserved (menu scroll / volume nudge later)
 *  MUSE_IMU_SHAKE_START          muse_state_set_mode(MUSE_MODE_DIZZY)   // planned mode ext.
 *  MUSE_IMU_SHAKE_END            muse_state_set_mode(MUSE_MODE_IDLE)
 *  MUSE_IMU_TAP                  muse_state_make_happy()                // pet the muse
 *  MUSE_IMU_DOUBLE_TAP           push-to-talk toggle (same path as PWR press)
 *  MUSE_IMU_PICKUP               set_mode(IDLE) + make_happy()          // "good morning"
 *  MUSE_IMU_STILL_30S            nothing (renderer already sees mode_t) — optional dim
 *  MUSE_IMU_STILL_5MIN           muse_state_set_mode(MUSE_MODE_OFF)     // auto-sleep
 *                                (or planned MUSE_MODE_SLEEPY)
 *  MUSE_IMU_MOVING / _STILL      internal gating only
 *
 * ---------------------------------------------------------------------------
 * HARDWARE VERIFICATION NEEDED ON ARRIVAL
 * ---------------------------------------------------------------------------
 *  1. I2C address: 0x6B assumed (0x6A fallback probed automatically).
 *  2. Axis mapping: which sensor axis points right/up/out-of-screen on this
 *     PCB. Lay the board flat face-up: expect az ~= +9.81. Tilt left edge
 *     down: expect ax > 0. Correct MUSE_IMU_REF_* below if mirrored.
 *  3. Tap thresholds: 1.8 g spike / quiet-window values are starting points;
 *     tune by tapping the actual enclosure (speaker vibration is the main
 *     false-trigger source; the Z-dominance + quiet-window requirements are
 *     the mitigation).
 *  4. waveshare/qmi8658 API details listed under VERIFY above.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Orientation of the board, derived from the gravity vector with hysteresis
 * (enter a new orientation at 45 deg, leave the old one at 30 deg). */
typedef enum {
    MUSE_IMU_TILT_UNKNOWN = 0,
    MUSE_IMU_FACE_UP,    /* screen facing up (board flat on table) */
    MUSE_IMU_FACE_DOWN,  /* screen facing down */
    MUSE_IMU_TILT_LEFT,  /* rolled left  (left edge down)  */
    MUSE_IMU_TILT_RIGHT, /* rolled right (right edge down) */
    MUSE_IMU_TILT_FWD,   /* pitched forward (top edge down) */
    MUSE_IMU_TILT_BACK,  /* pitched back   (top edge up)   */
} muse_imu_tilt_t;

/* Gesture / state events delivered to the callback. */
typedef enum {
    MUSE_IMU_EV_NONE = 0,
    MUSE_IMU_ORIENTATION, /* tilt changed; see event.tilt            */
    MUSE_IMU_MOVING,      /* motion energy rose above threshold      */
    MUSE_IMU_STILL,       /* motion energy quiet again               */
    MUSE_IMU_SHAKE_START, /* sustained high energy > 600 ms          */
    MUSE_IMU_SHAKE_END,   /* calm for 800 ms after a shake           */
    MUSE_IMU_TAP,         /* single Z-dominant spike + quiet window  */
    MUSE_IMU_DOUBLE_TAP,  /* two taps within 400 ms (TAP also fires) */
    MUSE_IMU_STILL_30S,   /* still for 30 s (fires once per still)   */
    MUSE_IMU_STILL_5MIN,  /* still for 5 min (fires once per still)  */
    MUSE_IMU_PICKUP,      /* still >= 3 s, now moving                */
} muse_imu_gesture_t;

typedef struct {
    muse_imu_gesture_t gesture;
    muse_imu_tilt_t tilt;  /* orientation snapshot at event time */
    uint32_t time_ms;      /* esp_timer-based millisecond timestamp */
} muse_imu_event_t;

typedef void (*muse_imu_cb_t)(const muse_imu_event_t *ev, void *ctx);

/*
 * Probe and configure the QMI8658 on an already-created shared I2C bus.
 * Idempotent. Returns ESP_OK even when the IMU is absent (some clones may
 * omit it) — check muse_imu_present(). Never blocks boot.
 */
esp_err_t muse_imu_init(i2c_master_bus_handle_t bus);

/* Undo init (stops the task if running). Safe to call when absent. */
void muse_imu_deinit(void);

/* Start/stop the 50 Hz gesture task. start() is a no-op (ESP_OK) when the
 * IMU is absent or the task is already running. */
esp_err_t muse_imu_start(void);
void muse_imu_stop(void);

/* Register the gesture callback (replaces any previous one). */
void muse_imu_set_callback(muse_imu_cb_t cb, void *ctx);

/* True when the IMU was found and configured. */
bool muse_imu_present(void);

/* Latest orientation / motion state (also delivered in every event). */
muse_imu_tilt_t muse_imu_tilt(void);
bool muse_imu_moving(void);

#ifdef __cplusplus
}
#endif
