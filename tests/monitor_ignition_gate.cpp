#include <cassert>
#include <cstdio>
#include "monitor_ignition_gate.h"
int main(){
  KlineIgnitionGate gate;
  assert(gate.state==IGN_UNKNOWN);
  gate.sample(true,true,0);
  gate.sample(true,true,2999);
  assert(gate.state==IGN_UNKNOWN);
  gate.sample(true,true,3000);
  assert(gate.state==IGN_HIGH);
  gate.sample(false,true,4000);
  gate.sample(false,false,5000);
  gate.sample(false,false,12999);
  assert(gate.state==IGN_HIGH); // brief dips should not look like ignition OFF
  gate.sample(false,false,13000);
  assert(gate.state==IGN_LOW&&gate.sawLow);
  gate.sample(true,true,14000);
  gate.sample(true,true,17000);
  assert(gate.state==IGN_HIGH);
  gate.sample(true,false,18000);
  assert(gate.state==IGN_HIGH); // don't trust conflicting channels
  std::puts("PASS ignition debounce: high stable 3s, low stable 8s, mixed ignored, re-arm");
}
