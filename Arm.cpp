#include "Arm.h"
#include <math.h>

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

bool Arm::begin() {
  pinMode(Cfg::PIN_EN, OUTPUT);
  digitalWrite(Cfg::PIN_EN, HIGH);                  // drivers OFF until everything is initialised
  pinMode(Cfg::PIN_ESTOP, INPUT_PULLUP);
  engine_.init();
  for (int i = 0; i < Cfg::N; i++)
    if (!joints_[i].begin(engine_, Cfg::PIN_STEP[i], Cfg::PIN_DIR[i], st_.joint[i])) return false;
  digitalWrite(Cfg::PIN_EN, LOW);                   // enable drivers -> arm must be in HOME pose now
  motorsOn_ = true;
  return true;
}

void Arm::applySettings() {
  haltImmediate();
  for (int i = 0; i < Cfg::N; i++) joints_[i].applyParams(st_.joint[i]);
}

bool Arm::atTarget() const {
  if (stopping_) return false;
  for (int i = 0; i < Cfg::N; i++) if (!joints_[i].atTarget()) return false;
  return true;
}

const char* Arm::stateName() const {
  if (estop_) return "estop";
  if (homing_) return "homing";
  return atTarget() ? "idle" : "moving";
}

/**
 * Synchronised move. Each joint has its own max speed/accel (gear ratios differ!), so the joint that
 * needs the LONGEST time is the "lead". Every other joint gets speed and acceleration scaled by
 * (its distance / lead distance). All trapezoids then have the same shape in time -> they finish together.
 */
void Arm::moveJoints(const float* q, float scale) {
  scale = clampf(scale, 0.05f, 1.0f);
  int32_t t[Cfg::N];
  float   d[Cfg::N];
  int     lead = -1;
  float   tLead = 0, vL = 1, aL = 1, dL = 1;

  for (int i = 0; i < Cfg::N; i++) {
    const Joint& j = joints_[i];
    d[i] = 0;
    t[i] = j.targetSteps();
    if (isnan(q[i])) continue;
    t[i] = j.angleToSteps(q[i]);
    d[i] = fabsf((float)(t[i] - j.steps()));
    if (d[i] < 1) continue;
    float tm = j.estimateTime(d[i], scale);
    if (tm > tLead) { tLead = tm; lead = i; vL = j.speedHz(scale); aL = j.accelHz(scale); dL = d[i]; }
  }

  stopping_ = false;
  for (int i = 0; i < Cfg::N; i++) {
    if (isnan(q[i])) continue;
    float ratio = (lead >= 0) ? d[i] / dL : 0.0f;
    joints_[i].moveTo(t[i], vL * ratio, aL * ratio);
  }
}

void Arm::home(float scale) {
  float q[Cfg::N];
  for (int i = 0; i < Cfg::N; i++) q[i] = 0;
  homing_ = true;
  moveJoints(q, scale);
}

void Arm::stopSmooth() {
  for (int i = 0; i < Cfg::N; i++) joints_[i].stopSmooth();
  stopping_ = true;
  homing_ = false;
}

void Arm::haltImmediate() {
  for (int i = 0; i < Cfg::N; i++) joints_[i].stopNow();
  stopping_ = false;
  homing_ = false;
}

void Arm::setZero() {
  haltImmediate();
  for (int i = 0; i < Cfg::N; i++) joints_[i].setZero();
}

void Arm::setMotors(bool on) {
  if (!on) haltImmediate();
  motorsOn_ = on;
  digitalWrite(Cfg::PIN_EN, on ? LOW : HIGH);
}

void Arm::estop() {
  estop_ = true;
  haltImmediate();                                  // drivers stay enabled: arm holds its pose
}

void Arm::update() {
  if (stopping_) {                                  // wait for the deceleration ramp to finish
    bool run = false;
    for (int i = 0; i < Cfg::N; i++) if (joints_[i].running()) run = true;
    if (!run) {
      for (int i = 0; i < Cfg::N; i++) joints_[i].syncTarget();
      stopping_ = false;
    }
  }
  if (homing_ && atTarget()) homing_ = false;

  // Physical E-STOP button (active LOW), small debounce
  btnCount_ = (digitalRead(Cfg::PIN_ESTOP) == LOW) ? (btnCount_ < 5 ? btnCount_ + 1 : btnCount_) : 0;
  if (btnCount_ == 3 && !estop_) estop();
}
