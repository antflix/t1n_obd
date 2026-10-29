#include <cassert>
#include "monitor_retry_policy.h"
int main(){
 MonitorRetryPolicy p;
 assert(p.onFailure()==15000);
 assert(p.onFailure()==45000);
 assert(p.onRealSample());
 assert(p.consecutive==0);
 assert(p.onFailure()==15000);
 assert(p.onFailure()==45000);
 assert(p.onFailure()==120000);
 assert(p.onFailure()==300000);
 assert(p.onRealSample());
 assert(p.onFailure()==15000);
}
