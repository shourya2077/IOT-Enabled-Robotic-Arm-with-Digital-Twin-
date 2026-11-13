#pragma once
/**
 * WebApi - HTTP front-end. Serves the page and a small JSON API. It contains NO decision logic:
 * every route just parses arguments and calls a Controller method.
 */
#include <Arduino.h>
#include <WebServer.h>
#include "Controller.h"
#include "NetManager.h"
#include "MqttLink.h"

class WebApi {
 public:
  WebApi(Controller& ctl, Settings& st, NetManager& net, MqttLink& mqtt)
      : ctl_(ctl), st_(st), net_(net), mqtt_(mqtt), server_(80) {}
  void begin();
  void update() { server_.handleClient(); }

 private:
  void done(const char* err);               // {"ok":true} or {"ok":false,"err":"..."}
  void sendJson(const JsonDocument& d);
  bool flag(const char* name, bool def);    // ?on=1 / ?on=0
  void routes();

  void getState();
  void getSettings();
  void getWaypoints();
  void postSettings();

  Controller& ctl_;
  Settings&   st_;
  NetManager& net_;
  MqttLink&   mqtt_;
  WebServer   server_;
};
