#pragma once
#include <stdint.h>
#include <math.h>
enum AlertTransition:uint8_t {ALERT_UNCHANGED,ALERT_OPENED,ALERT_RESOLVED};
struct AlertRule {
  bool enabled=false,high=true;
  float trigger=0,clear=0;
  uint32_t debounceMs=10000;
  uint8_t severity=1; // advisory=0, warning=1, critical=2
  bool valid()const {
    return isfinite(trigger)&&isfinite(clear)&&debounceMs<=600000
      &&severity<=2&&(high?clear<trigger:clear>trigger);
  }
};
struct AlertState {
  bool active=false,acknowledged=false,available=false,pending=false;
  uint32_t pendingAt=0,firstEpoch=0,lastEpoch=0;
  float evidence=0;
  AlertTransition update(const AlertRule&r,uint32_t now,bool fresh,float value){
    available=r.enabled&&fresh&&isfinite(value);
    if(!available){pending=false;return ALERT_UNCHANGED;}
    evidence=value;
    if(active){
      if(r.high?value<=r.clear:value>=r.clear){active=false;pending=false;return ALERT_RESOLVED;}
      return ALERT_UNCHANGED;
    }
    const bool abnormal=r.high?value>=r.trigger:value<=r.trigger;
    if(!abnormal){pending=false;return ALERT_UNCHANGED;}
    if(!pending){pending=true;pendingAt=now;}
    if((uint32_t)(now-pendingAt)<r.debounceMs)return ALERT_UNCHANGED;
    active=true;acknowledged=false;pending=false;return ALERT_OPENED;
  }
};
