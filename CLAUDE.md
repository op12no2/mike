# mike

Grumpy companion rover for Colin's grandchildren. Persona: Marvin-adjacent —
reluctant, put-upon, secretly devoted. The goal is theatre, not utility: he
detects the grandchildren, rolls up to them, and chats (locally-run speech,
eventually). Slow and torquey, never fast. Reliability and simplicity are
paramount; cost is not a constraint.

## Architecture

Two computers, one wire:

- **Raspberry Pi 5** — the brain. Speech recognition, camera/vision, the
  persona, all high-level behaviour. Runs 64-bit Raspberry Pi OS Lite
  (Debian Trixie), headless, user `xyzzy`, host `rpi`.
- **ESP32-S3** — the body. Real-time I/O only: PWM to the steering servo and
  ESC, sensor reads, and a watchdog that stops the rover if the brain goes
  quiet. Board: Espressif ESP32-S3-DevKitC-1, WROOM-1 module, N8R8 (8MB
  flash, 8MB PSRAM). Onboard addressable RGB LED on GPIO38 (GPIO48 on some
  board revisions).
- **Link**: USB, Pi ↔ devkit UART port (CP2102N → `/dev/ttyUSB0`, 115200
  8N1). This one cable carries power, the command protocol, and field
  reflashing, and stays in production. The plan is to later move the ESP
  console/logging to the board's second (native USB) port so UART0 becomes a
  pure command channel.

## Repo layout

- `esp/` — ESP-IDF project (firmware). C.
- `rpi/` — Pi-side programs. C.
- `protocol/` — Pi↔ESP protocol spec: one .md + one shared .h (planned).
- `docs/` — design notes and decisions.
- `hw/` — wiring, parts, eventually a carrier PCB.

## Building

ESP side (from `esp/`):

    . ~/esp/esp-idf/export.sh
    idf.py build flash

Pi side (from `rpi/`):

    make

**Port discipline: one reader at a time.** `idf.py monitor` and any Pi-side
program both want `/dev/ttyUSB0`. Never run both. Flashing also needs the
port free.

Python lives in `~/venvs/mike` (created with `--system-site-packages` for
future picamera2). Invoke by full path (`~/venvs/mike/bin/python`) — venvs
are never activated in this project. esptool for manual poking is there too;
`idf.py flash` uses IDF's own pinned copy.

## Hardware notes

- Chassis: Axial SCX10 III Base Camp 1/10 crawler kit (4WD, builder's kit).
- Power: battery → Pololu D36V50F5 5V/5.5A buck → Pi 5; ESP powered from the
  Pi's USB. Motor power via ESC on its own rail. Common ground everywhere;
  motor current never through USB ground.
- One power source at a time on the devkit: USB only — never feed the 5V pin
  simultaneously.
- `/boot/firmware/config.txt` has `usb_max_current_enable=1` (no USB-PD
  negotiation from the buck).

## Doctrine

- Plain C, hand-authored, KISS. No frameworks beyond ESP-IDF itself, no
  dependency sprawl. If it can be a flat file and a Makefile, it is.
- Unknown serial commands are answered with `Nothing happens.` — this is
  load-bearing culture (Colossal Cave; the human is `xyzzy`), not a TODO.
- Failures should degrade toward stillness: watchdog stops the motors when
  in doubt.
- No colour in terminal output. Ever.

## Status / roadmap

Done: full toolchain on the Pi, ESP-IDF cross-compile + flash loop proven,
UART0 echo firmware and Pi-side first-contact program exchanging bytes.

Next: design the command protocol (`protocol/`), command dispatch on the
ESP, servo/ESC PWM out, watchdog. Then the brain: speech, vision, persona.
