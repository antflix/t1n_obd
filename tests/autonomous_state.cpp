#include <cassert>
#include <cstdio>
#include "monitor_ownership.h"

int main(){
  MonitorOwnership mode;
  assert(!mode.enabled&&!mode.ownsSession);
  // Resume releases a manually connected session, then owns the UART before
  // the first automatic connection and throughout failures/backoff.
  assert(mode.changeMode(true,true));
  assert(mode.enabled&&!mode.ownsSession);
  assert(!mode.changeMode(true,false));
  assert(mode.enabled&&!mode.ownsSession);
  mode.ownsSession=true;
  assert(!mode.changeMode(true,true));
  assert(mode.enabled&&mode.ownsSession);
  // An explicit pause releases exactly an automatic session; the mode stays
  // manual indefinitely and no clock/hold expiry can reclaim it.
  assert(mode.changeMode(false,true));
  assert(!mode.enabled&&!mode.ownsSession);
  assert(!mode.changeMode(false,true));
  assert(!mode.enabled&&!mode.ownsSession);
  // Pausing disconnected/backoff operation needs no fabricated disconnect.
  assert(!mode.changeMode(true,false));
  assert(!mode.changeMode(false,false));
  // Restoring the persisted enabled flag after boot claims the UART without
  // claiming that an ECU session has already been established.
  mode.enabled=true;
  assert(mode.enabled&&!mode.ownsSession);
  assert(monitorAutomaticModuleQualified(0));
  assert(monitorAutomaticModuleQualified(2));
  assert(monitorAutomaticModuleQualified(3));
  const uint8_t unsupported[]={1,4,5,6,255};
  for(uint8_t id:unsupported)assert(!monitorAutomaticModuleQualified(id));
  std::puts("Automatic/manual UART ownership transitions and unattended module qualification PASS");
}
