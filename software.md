# Software

Single source of truth for the software design: what runs where, what
shape it takes, and where the reference material lives. The wire between
the two halves is `protocol.md`; the electrons are `hardware.md`.

## Shape

Two programs, one wire, and a deliberate asymmetry: all cleverness lives
on the Pi, the ESP stays boring. The body does real-time I/O and safety
only — anything that can be done on the Pi is done on the Pi. Safety
(throttle clamp, lease watchdog) is enforced in the body, where the
brain can't talk it out of it.

## Body firmware — `esp/`

ESP-IDF project, plain C, one source file: `main/mike_main.c`.

- Single-task design: `app_main` is the whole program — a UART byte pump
  feeding a line assembler, `dispatch()` for commands, `wd_check()` for
  the throttle lease, polled every pass (the 20 ms UART read timeout is
  the loop tick). No extra tasks, queues, or callbacks until a
  peripheral forces one.
- State is a handful of statics: `armed`, `thr`, `str`, `wdtrips`,
  `lease_end`.
- Logging is disabled (`esp_log_level_set` to NONE) because UART0 is the
  command channel; the console moves to the native-USB port later.
- Peripherals in use: UART0 (esp_driver_uart), RMT TX for the WS2812 LED
  (esp_driver_rmt, 10 MHz bytes encoder), esp_timer for the lease clock.
- Next: LEDC for servo/ESC PWM (50 Hz, microsecond pulses — the
  centre/endpoint/direction calibration constants live here, per
  protocol.md), then I2C master for INA219 / LSM6DSOX / SSD1306.
  `out_throttle()` / `out_steer()` are the stubs real PWM replaces.
- Component dependencies are declared in `main/CMakeLists.txt`
  (PRIV_REQUIRES); add there when a new driver is pulled in.

## Brain — `rpi/`

Plain C and a Makefile (`gcc -Wall -Wextra -O2`), no libraries beyond
libc. One program so far:

- `mike` — stdin → `/dev/ttyUSB0` → stdout line client. It owns the
  port-handling lore: raw mode, `CLOCAL`, `HUPCL` cleared so exiting
  doesn't drop DTR and reset the body, 0.1 s read slices, ~1 s reply
  timeout, and a 2 s settle plus input flush at open to absorb the
  auto-reset boot chatter.

Future brain programs (persona daemon, speech, vision) are all Pi-side
and speak the same protocol. The one-reader rule means there will be a
single port-owning process the rest talk to, never multiple openers.

## Toolchains

- ESP: ESP-IDF at `~/esp/esp-idf` (a v6.1-dev checkout), target
  `esp32s3`; `esptool` in `~/venvs/esp`. Build and flash commands are in
  CLAUDE.md.
- Pi: system gcc and make.

## References

ESP-IDF and board:

- Programming guide (ESP32-S3):
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
- Peripheral API reference (UART, RMT, LEDC, I2C):
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/
- ESP32-S3-DevKitC-1 user guide (pinout, the two USB ports, LED GPIO):
  https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide.html

I2C devices. We write our own drivers (doctrine: plain C, no dependency
sprawl) — the datasheet is the spec; Adafruit's guides and their
CircuitPython drivers are reference reading for init sequences, not
dependencies:

- INA219 power monitor — datasheet:
  https://www.ti.com/lit/ds/symlink/ina219.pdf
  — guide: https://learn.adafruit.com/adafruit-ina219-current-sensor-breakout
- LSM6DSOX IMU — ST product page (datasheet and app notes):
  https://www.st.com/en/mems-and-sensors/lsm6dsox.html
  — guide: https://learn.adafruit.com/lsm6dsox-and-ism330dhc-6-dof-imu
- SSD1306 OLED — no vendor page; datasheet is linked from the guide:
  https://learn.adafruit.com/monochrome-oled-breakouts
