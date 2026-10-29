#pragma once
#include <stdint.h>
#include <stddef.h>

// Pure, allocation-free DTC diff used by the ESP32 and the native host tests.
// Only call after a complete, successful ECU fault-read response.
struct MonitorDtcItem { uint16_t code; uint8_t status; };
struct MonitorDtcChange { uint16_t code; uint8_t status; uint8_t kind; };
enum MonitorDtcChangeKind : uint8_t {
  MON_DTC_BASELINE=0,
  MON_DTC_NEW=1,
  MON_DTC_STATUS_CHANGED=2,
  MON_DTC_NOT_REPORTED=3
};
inline size_t monitorDiffDtc(const MonitorDtcItem *previous,size_t oldCount,
                            bool priorSuccessful,
                            const MonitorDtcItem *current,size_t currentCount,
                            MonitorDtcChange *out,size_t outLimit){
  if(oldCount>32 || currentCount>32 || outLimit<oldCount+currentCount)return 0;
  size_t count=0;
  for(size_t i=0;i<currentCount;i++){
    bool found=false;
    for(size_t j=0;j<oldCount;j++)if(previous[j].code==current[i].code){
      found=true;
      if(priorSuccessful&&previous[j].status!=current[i].status)
        out[count++]={current[i].code,current[i].status,MON_DTC_STATUS_CHANGED};
      break;
    }
    if(!found)
      out[count++]={current[i].code,current[i].status,
                    (uint8_t)(priorSuccessful?MON_DTC_NEW:MON_DTC_BASELINE)};
  }
  if(priorSuccessful)for(size_t j=0;j<oldCount;j++){
    bool found=false;
    for(size_t i=0;i<currentCount;i++)if(previous[j].code==current[i].code){
      found=true;break;
    }
    if(!found)out[count++]={previous[j].code,previous[j].status,MON_DTC_NOT_REPORTED};
  }
  return count;
}
