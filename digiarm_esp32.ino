/**
 * DigiArm - 5-DOF stepper robotic arm  (ESP32 + RAMPS 1.4 + 5x DRV8825 + 5x NEMA17)
 *
 * This file only creates the objects and runs their update() methods. All logic is in the classes:
 *
 *   Settings    tunable values (gear ratio, speed, microstep, direction, limits, Wi-Fi, MQTT) + flash
 *   Joint       one stepper joint: steps <-> degrees, speed/accel
 *   Arm         5 joints + synchronised moves + E-STOP / enable
 *   Sequencer   learn (teach waypoints) / play
 *   Controller  command rules shared by web and MQTT  (the "brain")
 *   WebApi      phone dashboard + JSON API      MqttLink   MQTT protocol v1
 *   NetManager  Wi-Fi access point + station
 *
 * Libraries: FastAccelStepper (gin66), PubSubClient (Nick O'Leary), ArduinoJson v7 (Benoit Blanchon)
 * Board: "ESP32 Dev Module".  Wiring + usage: see README.md
 */
#include "Config.h"
#include "Settings.h"
#include "Arm.h"
#include "Sequencer.h"
#include "Controller.h"
#include "NetManager.h"
#include "MqttLink.h"
#include "WebApi.h"

Settings   settings;
Arm        arm(settings);
Sequencer  seq(arm, settings);
Controller ctl(settings, arm, seq);
NetManager net(settings);
MqttLink   mqtt(ctl, settings, net);
WebApi     web(ctl, settings, net, mqtt);

void setup() {
  Serial.begin(115200);
  settings.load();                         // flash -> RAM (defaults on first boot)
  if (!arm.begin()) {
    Serial.println("Stepper init failed - check Config.h pins");
    while (true) delay(1000);
  }
  seq.begin();                             // saved waypoints
  net.begin();
  mqtt.begin();
  web.begin();
  Serial.printf("Phone: join Wi-Fi \"%s\" -> http://%s\n", Cfg::AP_SSID, net.apIp().c_str());
}

void loop() {
  web.update();
  net.update();
  mqtt.update();
  arm.update();
  seq.update();
}
