#include "WebApi.h"
#include "WebPage.h"

void WebApi::begin() {
  routes();
  server_.begin();
}

void WebApi::done(const char* err) {
  String s = err ? String("{\"ok\":false,\"err\":\"") + err + "\"}" : String("{\"ok\":true}");
  server_.send(err ? 409 : 200, "application/json", s);
}

void WebApi::sendJson(const JsonDocument& d) {
  String s;
  serializeJson(d, s);
  server_.send(200, "application/json", s);
}

bool WebApi::flag(const char* name, bool def) {
  String v = server_.arg(name);
  return v.length() ? v != "0" : def;
}

void WebApi::routes() {
  // page + read-only data
  server_.on("/",             HTTP_GET, [this] { server_.send_P(200, "text/html", PAGE); });
  server_.on("/api/state",    HTTP_GET, [this] { getState(); });
  server_.on("/api/settings", HTTP_GET, [this] { getSettings(); });
  server_.on("/api/wps",      HTTP_GET, [this] { getWaypoints(); });

  // motion
  server_.on("/api/move", HTTP_GET, [this] {
    if (!server_.hasArg("v")) return done("bad request");
    done(ctl_.moveJoint(server_.arg("j").toInt(), server_.arg("v").toFloat()));
  });
  server_.on("/api/jog",   HTTP_GET, [this] { done(ctl_.jog(server_.arg("j").toInt(), server_.arg("d").toFloat())); });
  server_.on("/api/home",  HTTP_GET, [this] { done(ctl_.home()); });
  server_.on("/api/zero",  HTTP_GET, [this] { done(ctl_.zero()); });
  server_.on("/api/speed", HTTP_GET, [this] { done(ctl_.setSpeed(server_.arg("v").toFloat())); });

  // safety
  server_.on("/api/estop",  HTTP_GET, [this] { done(ctl_.estop()); });
  server_.on("/api/enable", HTTP_GET, [this] { done(ctl_.enable(flag("on", true))); });

  // teach / play
  server_.on("/api/learn", HTTP_GET, [this] { done(ctl_.learn(flag("on", true))); });
  server_.on("/api/save",  HTTP_GET, [this] { done(ctl_.savePose((uint16_t)constrain(server_.arg("dwell").toInt(), 0, 10000))); });
  server_.on("/api/wp/del",HTTP_GET, [this] { done(ctl_.deletePose(server_.arg("i").toInt())); });
  server_.on("/api/wp/go", HTTP_GET, [this] { done(ctl_.gotoPose(server_.arg("i").toInt())); });
  server_.on("/api/clear", HTTP_GET, [this] { done(ctl_.clearPoses()); });
  server_.on("/api/play",  HTTP_GET, [this] { done(ctl_.play(flag("loop", false))); });
  server_.on("/api/stop",  HTTP_GET, [this] { done(ctl_.stop()); });

  // misc
  server_.on("/api/mqttctl", HTTP_GET, [this] { ctl_.setRemoteControl(flag("on", true)); done(nullptr); });
  server_.on("/api/reboot",  HTTP_GET, [this] { done(nullptr); delay(300); ESP.restart(); });

  // settings (JSON body)
  server_.on("/api/settings", HTTP_POST, [this] { postSettings(); });
  server_.on("/api/reset",    HTTP_POST, [this] { done(ctl_.resetSettings()); });
}

void WebApi::getState() {
  JsonDocument d;
  ctl_.fillState(d);
  d["mq"] = mqtt_.statusText();
  d["ip"] = net_.staIp();
  sendJson(d);
}

void WebApi::getSettings() {
  JsonDocument d;
  st_.toJson(d);
  sendJson(d);
}

void WebApi::getWaypoints() {
  JsonDocument d;
  JsonArray a = d.to<JsonArray>();
  const Sequencer& s = ctl_.sequencer();
  for (int k = 0; k < s.count(); k++) {
    JsonObject o = a.add<JsonObject>();
    JsonArray q = o["q"].to<JsonArray>();
    for (int i = 0; i < Cfg::N; i++) q.add(s.waypoint(k).q[i]);
    o["d"] = s.waypoint(k).dwell;
  }
  sendJson(d);
}

void WebApi::postSettings() {
  JsonDocument d;
  if (deserializeJson(d, server_.arg("plain"))) return done("Bad JSON");
  done(ctl_.applySettings(d));
}
