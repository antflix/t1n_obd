#include <cassert>
#include <cstdio>
#include "monitor_dtc_diff.h"
int main(){
  MonitorDtcItem empty[1]={};
  MonitorDtcItem first[]={{0x1187,0x60},{0x1401,0x20}};
  MonitorDtcItem reordered[]={{0x1401,0x20},{0x1187,0x60}};
  MonitorDtcChange events[64]={};
  size_t n=monitorDiffDtc(empty,0,false,first,2,events,64);
  assert(n==2 && events[0].kind==MON_DTC_BASELINE && events[1].kind==MON_DTC_BASELINE);
  n=monitorDiffDtc(first,2,true,first,2,events,64);assert(n==0);
  n=monitorDiffDtc(first,2,true,reordered,2,events,64);assert(n==0);
  MonitorDtcItem changed[]={{0x1187,0x40},{0x1401,0x20}};
  n=monitorDiffDtc(first,2,true,changed,2,events,64);
  assert(n==1 && events[0].kind==MON_DTC_STATUS_CHANGED && events[0].code==0x1187);
  MonitorDtcItem added[]={{0x1187,0x60},{0x1401,0x20},{0x1612,0x40}};
  n=monitorDiffDtc(first,2,true,added,3,events,64);
  assert(n==1 && events[0].kind==MON_DTC_NEW && events[0].code==0x1612);
  MonitorDtcItem removed[]={{0x1401,0x20}};
  n=monitorDiffDtc(first,2,true,removed,1,events,64);
  assert(n==1 && events[0].kind==MON_DTC_NOT_REPORTED && events[0].code==0x1187);
  n=monitorDiffDtc(empty,0,true,removed,1,events,64);
  assert(n==1 && events[0].kind==MON_DTC_NEW);
  n=monitorDiffDtc(first,2,true,empty,0,events,64);
  assert(n==2 && events[0].kind==MON_DTC_NOT_REPORTED && events[1].kind==MON_DTC_NOT_REPORTED);
  n=monitorDiffDtc(first,2,true,reordered,2,events,1);
  assert(n==0); // bounded-buffer guard
  std::puts("DTC diff: baseline, repeat, reorder, status, new, removed, empty, buffer guard PASS");
}
