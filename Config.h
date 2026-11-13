#pragma once
/**
 * Config.h - COMPILE-TIME constants (changing these needs a re-flash).
 *
 * Rule of thumb used in this project:
 *   - Hardware wiring + access-point password  -> here (Config.h)
 *   - Everything you may want to tune          -> Settings (edited live on the web dashboard,
 *                                                 stored in flash). The values below named DEF_*
 *                                                 are only the FACTORY DEFAULTS for first boot / reset.
 */
#include <Arduino.h>

namespace Cfg {

constexpr int N = 5;                                   // number of joints

// ---- wiring (ESP32 GPIO -> RAMPS 1.4 driver socket) ----
//                                   J0  J1  J2  J3  J4
constexpr int PIN_STEP[N]        = { 13, 14, 25, 26, 27 };
constexpr int PIN_DIR[N]         = { 16, 17, 18, 19, 21 };
constexpr int PIN_EN             = 23;                 // one pin -> EN of all 5 drivers (active LOW)
constexpr int PIN_ESTOP          = 4;                  // optional push-button to GND

// ---- phone access point (always on) ----
constexpr const char* AP_SSID    = "DigiArm";
constexpr const char* AP_PASS    = "00000000";       // min 8 chars

// ---- hard limits of the step generator ----
constexpr float MAX_STEP_HZ      = 30000.0f;           // pulses/s per motor (DRV8825 + NEMA17 sane ceiling)
constexpr float MAX_ACCEL_HZ2    = 500000.0f;

// ---- misc ----
constexpr uint32_t TELEMETRY_MS  = 100;                // MQTT telemetry period
constexpr int      MAX_WAYPOINTS = 40;

// ---- factory defaults (editable later in the dashboard "Settings" tab) ----
constexpr const char* DEF_NAME[N]  = { "J0 Base Yaw", "J1 Shoulder", "J2 Elbow", "J3 Wrist Pitch", "J4 Wrist Roll" };
constexpr float DEF_GEAR[N]        = { 1.0f, 50.0f, 26.85f, 26.85f, 1.0f };   // total reduction (gearbox x belt)
constexpr float DEF_MIN[N]         = { -180, -30, -105, -100, -180 };         // deg, relative to power-on pose
constexpr float DEF_MAX[N]         = {  180, 110,   85,  100,  180 };
constexpr float DEF_VMAX[N]        = {   60,  20,   25,   30,   90 };         // deg/s at 100 % speed
constexpr float DEF_AMAX[N]        = {  120,  40,   50,   60,  200 };         // deg/s^2 at 100 % speed
constexpr float DEF_STEPS_PER_REV  = 200.0f;           // 1.8 deg motor
constexpr int   DEF_MICROSTEP      = 16;               // must match the MS jumpers on the RAMPS
constexpr float DEF_SPEED          = 0.5f;             // 50 % at power-on

constexpr const char* DEF_TOPIC_TELEMETRY = "arm/telemetry";
constexpr const char* DEF_TOPIC_COMMAND   = "arm/command";
constexpr const char* DEF_TOPIC_STATUS    = "arm/status/device";
constexpr uint16_t    DEF_MQTT_PORT       = 1883;

}  // namespace Cfg
