#pragma once
#include <stdint.h>

// One retry owner. Never reconfigure a station while its connection is pending.
// Disconnect only the station; keep the recovery AP and stored credentials.
struct WifiRecovery {
  enum Phase { Connecting, Online, Backoff } phase = Connecting;
  enum Action { None, Disconnect, Connect };
  uint32_t since = 0, waitMs = 30000;
  uint8_t failures = 0;
  static constexpr uint32_t connectionBudgetMs = 30000;

  void started(uint32_t now) { phase = Connecting; since = now; }
  void settingsChanged(uint32_t now) {
    phase = Backoff; since = now; waitMs = 250; failures = 0;
  }
  uint32_t nextInMs(uint32_t now) const {
    if (phase == Online) return 0;
    uint32_t limit = phase == Connecting ? connectionBudgetMs : waitMs;
    uint32_t elapsed = now - since;
    return elapsed >= limit ? 0 : limit - elapsed;
  }
  Action tick(uint32_t now, bool connected, bool configured) {
    if (!configured) return None;
    if (connected) {
      phase = Online; failures = 0; return None;
    }
    if (phase == Online || (phase == Connecting && !nextInMs(now))) {
      if (failures < 5) ++failures;
      waitMs = 30000u << (failures - 1);
      if (waitMs > 300000) waitMs = 300000;
      phase = Backoff; since = now;
      return Disconnect;
    }
    if (phase == Backoff && !nextInMs(now)) {
      started(now); return Connect;
    }
    return None;
  }
};
