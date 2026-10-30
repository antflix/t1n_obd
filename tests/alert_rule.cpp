#include <cassert>
#include <iostream>
#include "alert_rule.h"
int main(){
 AlertRule r;r.enabled=true;r.trigger=100;r.clear=95;r.debounceMs=2000;
 assert(r.valid());AlertState s;
 assert(s.update(r,100,true,101)==ALERT_UNCHANGED);
 assert(s.update(r,2099,true,101)==ALERT_UNCHANGED);
 assert(s.update(r,2100,true,101)==ALERT_OPENED);
 s.acknowledged=true;assert(s.update(r,2200,true,97)==ALERT_UNCHANGED&&s.active);
 assert(s.update(r,2300,false,0)==ALERT_UNCHANGED&&s.active&&!s.available);
 assert(s.update(r,2400,true,94)==ALERT_RESOLVED);
 s.update(r,2500,true,101);assert(s.update(r,4500,true,101)==ALERT_OPENED&&!s.acknowledged);
 s={};s.update(r,100,true,101);s.update(r,500,false,101);s.update(r,2000,true,101);assert(s.update(r,3999,true,101)==ALERT_UNCHANGED);
 assert(s.update(r,4000,true,101)==ALERT_OPENED);
 r.high=false;r.trigger=10;r.clear=12;assert(r.valid());s={};s.update(r,0xffffff00,true,9);assert(s.update(r,1744,true,9)==ALERT_OPENED);
 assert(s.update(r,2000,true,12)==ALERT_RESOLVED);
 r.enabled=false;s={};assert(s.update(r,3000,true,0)==ALERT_UNCHANGED&&!s.available);
 r.clear=r.trigger;assert(!r.valid());
 std::cout<<"Alert debounce, hysteresis, acknowledgment, invalid data, recurrence and uptime rollover PASS\n";
}
