# T1N OBD

ESP32 CR2/EDC15C6 diagnostic dashboard for the 2001-2003 OM612 T1N Sprinter.

Firmware source: `firmware/t1n_obd/t1n_obd.ino`

Pushes to `main` automatically build an ESP32 OTA application image and publish a moving `latest` release containing `firmware.bin`, `firmware.sha256`, and `manifest.json`.

The ESP32 web interface supports both manual `.bin` upload and one-button download/install of the latest GitHub-built firmware.
