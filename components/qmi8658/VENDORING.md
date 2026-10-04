# Why this component is vendored

Source: `waveshare/qmi8658` v1.0.0 from the ESP-IDF component registry
(https://github.com/waveshareteam/Waveshare-ESP32-components, Apache-2.0 —
see `license.txt`).

Two fixes vs. upstream v1.0.0:

1. **ESP-IDF v6 build fix.** `include/qmi8658.h` includes
   `driver/i2c_master.h`, which in IDF v6 lives in the `esp_driver_i2c`
   component. Upstream's `CMakeLists.txt` only declares `REQUIRES "driver"`,
   so the v6 build fails with `fatal error: driver/i2c_master.h: No such
   file or directory`. Fixed by adding `"esp_driver_i2c"` to REQUIRES.

2. **Nothing else.** The driver API is unchanged from v1.0.0, which is what
   `muse_imu.c` was written and verified against (`qmi8658_dev_t`,
   `qmi8658_data_t`, `qmi8658_init`, ...).

`apply.sh` copies this directory to `$SDK/components/qmi8658/`. A component
in the project's `components/` directory takes precedence over the
`waveshare/qmi8658` registry dependency declared in
`components/muse/idf_component.yml` (verified: the component manager prints
`NOTE: Using component placed at .../components/qmi8658 for dependency
"waveshare/qmi8658"` and skips the download), so this also makes the build
work offline and immune to registry changes.

If Waveshare ever publishes a v1.x with the IDF v6 fix, this vendored copy
can be dropped and the registry pin used again — but check the API first:
v2.x renamed types and functions.
