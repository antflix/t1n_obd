#pragma once
#include <stdint.h>

// Passive TWO-channel ignition heuristic: CR2 and EGS RXD levels.
// Both lines must agree continuously; mixed/transitioning lines are unknown.
// IMPORTANT: RXD GPIOs have pull-ups. A HIGH reading alone does NOT prove
// the key is on, and ignition-off behavior must be qualified on the vehicle.
enum KlineIgnitionState : uint8_t { IGN_UNKNOWN=0, IGN_LOW=1, IGN_HIGH=2 };
struct KlineIgnitionGate {
  uint8_t state=IGN_UNKNOWN;
  uint8_t candidate=IGN_UNKNOWN;
  uint32_t candidateSince=0;
  uint32_t lastTransition=0;
  bool sawLow=false;
  uint8_t sample(bool cr2High, bool egsHigh, uint32_t now){
    uint8_t raw= cr2High&&egsHigh?IGN_HIGH:
                 !cr2High&&!egsHigh?IGN_LOW:IGN_UNKNOWN;
    if(raw!=candidate){
      candidate=raw;
      candidateSince=now;
      return state;
    }
    if(raw!=IGN_UNKNOWN && state!=raw){
      uint32_t required=raw==IGN_LOW?8000UL:3000UL;
      if((uint32_t)(now-candidateSince)>=required){
        state=raw;
        lastTransition=now;
        if(raw==IGN_LOW)sawLow=true;
      }
    }
    return state;
  }
};
