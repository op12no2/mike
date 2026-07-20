# Mike

Grumpy companion rover for Colin's grandchildren. Persona: Marvin-adjacent —
reluctant, put-upon, secretly devoted. The goal is theatre, not utility: he
detects the grandchildren, rolls up to them, and chats (locally-run speech,
eventually). Slow and torquey, never fast. Reliability and simplicity are
paramount; cost is not a constraint. Beach-proof as far as possible.

## Architecture

Two computers, one wire:

- **Raspberry Pi 5** — the brain. Speech recognition, camera/vision, the
  persona, all high-level behaviour. Runs 64-bit Raspberry Pi OS Lite
  (Debian Trixie), headless, user `xyzzy`, host `rpi`.
- **ESP32-S3** — the body. Real-time I/O only: PWM to the steering servo and
  ESC, sensor reads, and a watchdog that stops the rover if the brain goes
  quiet. Board: Espressif ESP32-S3-DevKitC-1, WROOM-1 module, N8R8 (8MB
  flash, 8MB PSRAM). Onboard addressable RGB LED on GPIO38 (GPIO48 on some
  board revisions). I2C sensors.
- **Link**: USB, Pi ↔ devkit UART port (CP2102N → `/dev/ttyUSB0`, 115200
  8N1). This one cable carries power, the command protocol, and field
  reflashing, and stays in production. The plan is to later move the ESP
  console/logging to the board's second (native USB) port so UART0 becomes a
  pure command channel.

## Repo layout

- `esp/` — ESP-IDF project (firmware). C.
- `rpi/` — Pi-side programs. C.
- `protocol.md` — the wire protocol spec, single source of truth; firmware
  and Pi-side code follow it.
- `software.md` — the software design: shape, toolchains, driver
  references.
- `hardware.md` — the electrical design, single source of truth.
- `parts.md` — the shopping list.

## Building

ESP side (from `esp/`):

    . ~/esp/esp-idf/export.sh
    idf.py build flash

Pi side (from `rpi/`):

    make

**Port discipline: one reader at a time.** `idf.py monitor` and any Pi-side
program both want `/dev/ttyUSB0`. Never run both. Flashing also needs the
port free.

## Testing

- `~/venvs/esp/bin/esptool`
- `picocom -b 115200 --omap crlf --echo /dev/ttyUSB0` (alias `talk`) — talk to the body
  by hand; exit with Ctrl-A then Ctrl-X. Both flags are required: without
  `--omap crlf` Enter sends `\r` (which the firmware ignores — looks like
  a hang), without `--echo` you can't see your own typing. `minicom` is
  also installed. Either counts as the port's one reader.
- `rpi/mike` reads protocol lines on stdin and prints the replies:
  `printf 'ping\ntel\n' | ./mike` — good for scripted tests.

## Hardware notes

- Chassis: Axial SCX10 III Base Camp 1/10 crawler kit (4WD, builder's kit).
- Power in one line: battery → Y split → [pull loop → Fusion SE ESC] and
  [fuse → INA219 → buck → Pi 5 → USB → ESP32].
- The electrical design (topology, gauges, pull loop, fuse, grounding,
  I2C map) lives in `hardware.md` — single source of truth; consult it
  before touching wiring.
- One power source at a time on the devkit: USB only — never feed the 5V pin
  simultaneously.
- `/boot/firmware/config.txt` has `usb_max_current_enable=1` (no USB-PD
  negotiation from the buck).

## Session end ("off")

There is no power switch — deliberate decision, don't relitigate. A switch
in the main line would need ~50A rating and corrodes in salt air, and it
solves neither real problem: the Pi needs a *clean* shutdown (software),
and the LiPo must be physically unplugged after every session regardless
(standby drain through buck/ESC will over-discharge and kill it). The
ritual:

1. Kid pulls the XT60 loop (ESC branch) — motion dead by physics; Mike
   stays up and grumbles about it.
2. Kid says goodnight — clean poweroff with a farewell line. Until speech
   exists: a protocol command or the Pi 5 power button. A chunky external
   momentary button on the shell, wired to the Pi 5's power-button pads,
   is the interim and permanent fallback (parts TBD at shell layout).
3. Adult unplugs the battery. LiPo discipline: charge in a LiPo bag,
   storage-charge if idle more than a few days, never store connected.

## Doctrine

- Plain C, hand-authored, KISS. No frameworks beyond ESP-IDF itself, no
  dependency sprawl. If it can be a flat file and a Makefile, it is.
- Unknown serial commands are answered with `Nothing happens.` — this is
  load-bearing culture (Colossal Cave; the human is `xyzzy`), not a TODO.
- Failures should degrade toward stillness: watchdog stops the motors when
  in doubt.
- Design decisions land in the spec files — `protocol.md`, `hardware.md`,
  `software.md` — in the same commit as the change they describe.
  CLAUDE.md stays a summary with pointers; `parts.md` stays a shopping
  list.
- No colour in terminal output. Ever.


## Status / roadmap

State as of 2026-07-20. Physical: chassis kit assembled; no electronics
mounted yet; power-harness parts (XT60s, fuse holder, wire, heat shrink)
on order. Software: full toolchain on the Pi, ESP-IDF cross-compile +
flash loop proven, protocol v1 specced (`protocol.md`) and implemented —
command dispatch on the ESP (ping/ver/arm/disarm/drv/stop/led/tel),
throttle-lease watchdog, onboard LED via RMT, Pi-side line client.

Next on the bench: build the power harness when parts arrive (see
`hardware.md`), mount the electronics. Next in firmware: servo/ESC PWM
out (LEDC), I2C sensors (INA219, LSM6DSOX) and real telemetry keys, OLED
status display. Then the brain: speech, vision, persona.
