#include "Joint.h"
#include <math.h>

bool Joint::begin(FastAccelStepperEngine& engine, int stepPin, int dirPin, const JointParams& p) {
  stp_ = engine.stepperConnectToPin(stepPin);
  if (!stp_) return false;
  dirPin_ = dirPin;
  p_ = p;
  spd_ = p_.stepsPerDeg();
  stp_->setDirectionPin(dirPin_, !p_.invert);   // 2nd arg: "HIGH level counts up"
  stp_->setCurrentPosition(0);
  target_ = 0;
  return true;
}

void Joint::applyParams(const JointParams& np) {
  if (np.invert != p_.invert) {
    // Flipping the direction while keeping the PHYSICAL pose means the step counter must change sign.
    int32_t pos = stp_->getCurrentPosition();
    stp_->forceStopAndNewPosition(-pos);
    target_ = -pos;
    stp_->setDirectionPin(dirPin_, !np.invert);
  }
  p_ = np;
  spd_ = p_.stepsPerDeg();   // steps stay; angle is re-derived with the new gear ratio / microstep
}

float Joint::clampAngle(float deg) const {
  return deg < p_.minDeg ? p_.minDeg : (deg > p_.maxDeg ? p_.maxDeg : deg);
}
int32_t Joint::angleToSteps(float deg) const { return lroundf(clampAngle(deg) * spd_); }

float Joint::speedHz(float scale) const { return fminf(p_.maxSpeed * scale * spd_, Cfg::MAX_STEP_HZ); }
float Joint::accelHz(float scale) const { return fminf(p_.maxAccel * scale * spd_, Cfg::MAX_ACCEL_HZ2); }

// Trapezoid (reaches cruise speed) or triangle (too short to reach it) move time.
float Joint::estimateTime(float d, float scale) const {
  if (d <= 0) return 0;
  float v = speedHz(scale), a = accelHz(scale);
  if (d >= v * v / a) return d / v + v / a;
  return 2.0f * sqrtf(d / a);
}

void Joint::moveTo(int32_t t, float vHz, float aHz) {
  vHz = fminf(fmaxf(vHz, 1.0f), Cfg::MAX_STEP_HZ);
  aHz = fminf(fmaxf(aHz, 1.0f), Cfg::MAX_ACCEL_HZ2);
  stp_->setSpeedInHz((uint32_t)vHz);
  stp_->setAcceleration((int32_t)aHz);
  target_ = t;
  stp_->moveTo(t);
}

void Joint::stopSmooth() { stp_->stopMove(); }

void Joint::stopNow() {
  int32_t p = stp_->getCurrentPosition();
  stp_->forceStopAndNewPosition(p);
  target_ = p;
}

void Joint::setZero() {
  stp_->forceStopAndNewPosition(0);
  target_ = 0;
}
