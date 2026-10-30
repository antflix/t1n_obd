#include <cassert>
#include "wifi_recovery.h"

int main() {
  WifiRecovery p;
  p.started(0);
  assert(p.tick(29999, false, true) == WifiRecovery::None);
  assert(p.tick(30000, false, true) == WifiRecovery::Disconnect);
  assert(p.tick(59999, false, true) == WifiRecovery::None);
  assert(p.tick(60000, false, true) == WifiRecovery::Connect);
  assert(p.tick(89999, false, true) == WifiRecovery::None);
  assert(p.tick(90000, false, true) == WifiRecovery::Disconnect);
  assert(p.waitMs == 60000);
  assert(p.tick(149999, false, true) == WifiRecovery::None);
  assert(p.tick(150000, false, true) == WifiRecovery::Connect);
  for (int i = 0; i < 20; ++i) {
    uint32_t timeout = p.since + p.connectionBudgetMs;
    assert(p.tick(timeout, false, true) == WifiRecovery::Disconnect);
    assert(p.waitMs <= 300000);
    assert(p.tick(timeout + p.waitMs, false, true) == WifiRecovery::Connect);
  }
  assert(p.waitMs == 300000);
  assert(p.tick(p.since + 1, true, true) == WifiRecovery::None);
  assert(p.failures == 0 && p.phase == WifiRecovery::Online);
  assert(p.tick(p.since + 2, false, true) == WifiRecovery::Disconnect);
  assert(p.waitMs == 30000);
  p.settingsChanged(UINT32_MAX - 100);
  assert(p.tick(148, false, true) == WifiRecovery::None);
  assert(p.tick(149, false, true) == WifiRecovery::Connect);
  p.started(UINT32_MAX - 100);
  assert(p.tick(29898, false, true) == WifiRecovery::None);
  assert(p.tick(29899, false, true) == WifiRecovery::Disconnect);
  assert(p.tick(100000, false, false) == WifiRecovery::None);
}
