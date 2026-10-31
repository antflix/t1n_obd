#pragma once
#include <stdint.h>

// Requested operating mode owns the UART even while disconnected/backing off.
// Session ownership records which session an explicit pause must release.
struct MonitorOwnership {
  bool enabled=false;
  bool ownsSession=false;

  // Return whether the caller must release the current diagnostic session.
  // Repeating a mode request must not reset an already-running logger.
  bool changeMode(bool automatic,bool sessionActive){
    if(enabled==automatic)return false;
    const bool release=sessionActive&&(automatic||ownsSession);
    enabled=automatic;
    ownsSession=false;
    return release;
  }
};

// Canonical module IDs: CR2=0, ABS=1, EGS=2, HZR=3, EWM=4, KI=5.
// ABS remains excluded until its physical channel is separately qualified.
constexpr bool monitorAutomaticModuleQualified(uint8_t module){
  return module==0||module==2||module==3;
}
