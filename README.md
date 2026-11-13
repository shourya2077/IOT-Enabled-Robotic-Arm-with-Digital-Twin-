# DigiArm ESP32 firmware (OOP version)

Open `digiarm_esp32/digiarm_esp32.ino` in Arduino IDE (keep all files in the same folder).
Libraries: **FastAccelStepper** (gin66), **PubSubClient**, **ArduinoJson v7**. Board: ESP32 Dev Module.

## What lives where
| Need to change... | Where | Re-flash? |
|---|---|---|
| gear ratio, motor steps/rev, microstep, direction, limits, max speed/accel, joint names | dashboard -> Settings tab | no |
| Wi-Fi client (SSID/password), MQTT broker/port/user/password, topics | dashboard -> Settings tab | no |
| default speed at power-on | Settings tab (saves the Speed slider value) | no |
| GPIO pins, AP name/password, step-rate ceiling | `Config.h` | yes |

Everything editable is stored in flash (Preferences) and survives power cycles. "Factory reset" restores `Config.h` defaults.

## Classes (one job each)
```
 phone browser ──HTTP──> WebApi ─┐
                                 ├──> Controller ──> Arm ──> Joint x5 ──> FastAccelStepper (hardware pulses)
 MQTT broker  ──────────> MqttLink┘        │
                                           └──> Sequencer (learn / play, waypoints in flash)
 Settings (shared, in flash)  <── read by every class;  NetManager = Wi-Fi AP + station
```
| File | Class | Job |
|---|---|---|
| `Settings.h/.cpp` | `Settings` | all tunables, JSON in/out, flash save/load, validation |
| `Joint.h/.cpp` | `Joint` | one joint: degrees <-> steps, speed/accel, limits |
| `Arm.h/.cpp` | `Arm` | 5 joints, **time-synchronised moves**, enable pin, E-STOP |
| `Sequencer.h/.cpp` | `Sequencer` | waypoints, learn mode, playback state machine |
| `Controller.h/.cpp` | `Controller` | the rules ("is this command allowed now?") shared by web + MQTT |
| `WebApi.h/.cpp`, `WebPage.h` | `WebApi` | HTTP routes + the phone page (HTML/JS in flash) |
| `MqttLink.h/.cpp` | `MqttLink` | MQTT protocol v1 (telemetry out, commands in) |
| `NetManager.h/.cpp` | `NetManager` | access point + joining your router |

**Add a new command:** (1) add a method to `Controller`; (2) one line in `WebApi::routes()`; (3) one `else if` in `MqttLink::onMessage()`.

## Synchronised motion (Arm::moveJoints)
Joints have different gear ratios, so each one has a different step count and max step rate. The joint that needs the
longest time is the *lead*. Every other joint gets speed and acceleration scaled by `distance / lead distance`, so all
trapezoidal profiles have the same shape in time and finish together (checked: five-joint test finishes within 0.02 s).

## Wiring (ESP32 -> RAMPS 1.4, no Mega fitted)
| Joint | RAMPS slot | STEP | DIR |
|---|---|---|---|
| J0 | X | GPIO13 -> D54 (A0) | GPIO16 -> D55 (A1) |
| J1 | Y | GPIO14 -> D60 (A6) | GPIO17 -> D61 (A7) |
| J2 | Z | GPIO25 -> D46 | GPIO18 -> D48 |
| J3 | E0 | GPIO26 -> D26 | GPIO19 -> D28 |
| J4 | E1 | GPIO27 -> D36 | GPIO21 -> D34 |

* GPIO23 -> EN of all five drivers (D38, A2/D56, A8/D62, D24, D30). Active LOW.
* ESP32 GND <-> RAMPS GND. Optional E-STOP button: GPIO4 to GND.
* DRV8825 1/16 microstep = only the **MS3** jumper under each driver (all three = 1/32). Whatever you choose must match the Microstep field in Settings.
* DRV8825 current: I = 2 x Vref (0.1 ohm sense resistors; check your module). 12 V supply; RAMPS motor input is rated about 5 A.

## First run
1. Flash, power the arm **in its HOME pose** (angle 0 = power-on pose; no encoders or limit switches).
2. Phone -> Wi-Fi `DigiArm` / `digiarm123` -> `http://192.168.4.1`.
3. Settings tab: enter real gear ratios, microstep, then use the **-5 / +5 deg** test buttons per joint. Flip *Reverse direction* if it moves the wrong way. Verify that 5 deg on the screen is 5 deg on the arm; if not, the ratio is wrong.
4. Enter Wi-Fi + broker, Save. The header shows the MQTT status; the dashboard uses the broker's WebSocket port, the ESP32 the TCP port.
5. Keep speed at 10-20 % until every joint is verified.

## Behaviours to know
* E-STOP stops motion but keeps the drivers energised (the arm does not drop). Use a real power-cutting E-stop in the 12 V line for people-safety.
* "Motors OFF" releases the arm; it may fall and the position count will be wrong afterwards -> pose it at home and press *Set home here*.
* Changing gear ratio or microstep keeps the step counter, so displayed angles are recomputed; re-teach saved poses afterwards. Flipping *Reverse direction* negates the counter so the physical pose is preserved.
* The *allow remote control* tick on the phone page blocks MQTT motion commands (E-STOP over MQTT always works).
