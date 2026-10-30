# Pi display supervisor

Installed on the Pi at `~/.local/share/t1n-kiosk/`:

- `esp-kiosk.py`: one Chromium process, scanner recovery and renderer supervision.
- `browser_health.py`: standard-library read-only renderer heartbeat.
- `offline.html`: local unavailable-state display.

`~/.local/bin/esp-kiosk.sh` starts the supervisor from the existing labwc autostart. Copy both Python files when updating the launcher. Preserve the existing labwc touchscreen configuration and keep old launcher/assets in a private backup directory.

The dedicated Chromium profile is `~/.cache/t1n-kiosk-profile`. This display-only profile uses basic password storage to avoid the desktop keyring prompt; do not use it for saved credentials. Wayland and software rendering are selected for the current Pi 3B/DSI installation. No graphics drivers are modified.

Chromium's renderer heartbeat uses a random loopback-only diagnostics port, with no additional package or server dependency. A 45-second startup grace and three consecutive failed checks avoid rapid relaunches. Scanner HTTP reachability and renderer health are independent. The loaded dashboard handles routine network interruptions; the supervisor reopens it when the ESP32 returns.

The supervisor sends only read-only ESP32 requests. It does not control diagnostics or store measurements. Default scanner URL is `http://192.168.8.168/pi`; `T1N_SCANNER_URL` can override it.

Validated on the physical Pi: offline rendering, renderer crash recovery, automatic boot, single browser instance, preserved SSH and unchanged labwc configuration. Live page recovery and physical touchscreen use still require the scanner to be reachable. See the repository validation report for exact evidence and outstanding scanner faults.
