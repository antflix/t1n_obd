#pragma once
#include <stdint.h>
#include <math.h>

// Preserve the deployed 32-byte layout and FNV-1a integrity checksum. Older
// notes called it CRC; changing algorithms here would discard existing data.
struct HistoryRecord {
  uint32_t serial, epoch, key;
  float minimum, maximum, average;
  uint32_t samples, checksum;
};
static_assert(sizeof(HistoryRecord)==32,"History record size changed");
inline uint32_t historyChecksum(const HistoryRecord &r){
  uint32_t h=2166136261UL;
  const uint8_t *p=(const uint8_t*)&r;
  for(unsigned n=0;n<sizeof(r)-4;n++)h=(h^p[n])*16777619UL;
  return h;
}
inline bool historyValid(const HistoryRecord &r){
  return r.serial&&r.serial!=0xffffffffUL&&r.epoch>=1609459200UL
    &&r.epoch<4102444800UL&&r.key&&r.samples
    &&isfinite(r.minimum)&&isfinite(r.maximum)&&isfinite(r.average)
    &&r.minimum<=r.average&&r.average<=r.maximum
    &&r.checksum==historyChecksum(r);
}
inline bool historyInWindow(const HistoryRecord &r,int tier,uint32_t after,uint32_t now){
  if(r.epoch<after||r.epoch>now)return false;
  const uint32_t age=now-r.epoch;
  return tier==0?age<=3600UL:tier==1?age>3600UL&&age<=86400UL:age>86400UL;
}
// Descending order used by bounded query selection and its continuation cursor.
inline bool historyNewer(const HistoryRecord &a,int at,const HistoryRecord &b,int bt){
  if(a.epoch!=b.epoch)return a.epoch>b.epoch;
  if(at!=bt)return at<bt;
  return a.serial>b.serial;
}
