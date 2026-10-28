#pragma once
#include <stdint.h>
// Counts only *consecutive* failures since a successfully persisted real
// engine measurement. No rate limit over an arbitrary 20-minute period.
// Keep retrying indefinitely with a bounded cooldown; never permanently
// turn logging off just because an ECU is temporarily unavailable.
struct MonitorRetryPolicy {
  uint8_t consecutive=0;
  uint32_t recoveries=0;
  uint32_t onFailure() {
    if(consecutive<255)consecutive++;
    return consecutive==1?15000UL:
           consecutive==2?45000UL:
           consecutive==3?120000UL:300000UL;
  }
  bool onRealSample() {
    if(!consecutive)return false;
    consecutive=0;
    recoveries++;
    return true;
  }
};
