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
  (esp_driver_rmt, 10 MHz bytes encoder), esp_timer for the lease clock,
  I2C master (esp_driver_i2c) on GPIO8/9 at 400 kHz — LSM6DSOX driver:
  WHOAMI probe + config at boot, 12-byte gyro+accel burst per `tel`,
  pitch/roll computed from accel (float internally, integers on the
  wire). An absent or unplugged IMU degrades to `imu=0`, never an error.
- Next: LEDC for servo/ESC PWM (50 Hz, microsecond pulses — the
  centre/endpoint/direction calibration constants live here, per
  protocol.md), then INA219 and SSD1306 on the same I2C bus.
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

- `voice` — the double-act performer: scans `bits/`, fires bits on
  stdin events, self-schedules idle grumbles on a mood clock. Details
  in the Voice section below.

Future brain programs (persona daemon, speech, vision) are all Pi-side
and speak the same protocol. The one-reader rule means there will be a
single port-owning process the rest talk to, never multiple openers.

## Voice — the double act

Mike's first behaviour (before ears or vision): a two-character voice
act. MIKE, articulate and put-upon, argues with the body's voice — the
nameless BODY, all-caps monotone, barely intelligible on purpose. Both
are rendered on the Pi through the one mono speaker (`hardware.md`,
Audio); the ESP32 never touches audio. What keeps it honest: BODY lines
are triggered by real protocol data, so the sketch is a dramatization of
actual wire traffic.

### Engines

- MIKE — Piper (neural TTS, local, faster than real-time on the Pi 5).
  Run resident: text lines in on stdin, raw audio streamed to ALSA —
  model loads once, response is effectively instant.
- BODY — espeak-ng (formant synthesis, no model). Spawned per line;
  it's instant anyway. The `-a`/`-s`/`-p` flags (amplitude, speed,
  pitch) are the escalation levers: same line again, LOUDER and
  SLOWER, then spelled out.
- No runtime LLM — deliberate. Curated lines never miss; a small model
  misses in front of the audience, and wit is what tiny models are
  worst at. Generation happens offline (Claude as writer's room), the
  rover ships deterministic flat files. Revisit only if/when a local
  model exists that is reliably in-character on this class of hardware.

### Content — one file per bit

Chit-chat lives in `rpi/bits/`, one plain-text file per *bit* (a gag,
sketch, or pool of one-liners). The program scans the directory at
start; a new gag is a new file, no code change. The format is the
contract with human contributors — writable in a text editor with no
programming:

    # battery-low.bit — the body nags, Mike deflects
    trigger: battery_low
    mode: script
    cooldown: 3600

    BODY: BATTERY LOW.
    MIKE: I am perfectly aware of that.
    BODY: BATTERY LOW.
    MIKE: You are not {helping|contributing|part of the solution}.
    BODY: CRITICAL BATTERY.
    MIKE: Now you're just being {dramatic|operatic|hysterical}.

- `#` comments; `key: value` header; blank line; then `SPEAKER: text`
  lines.
- `mode: script` plays the lines in order (a sketch). `mode: pool`
  picks one line per firing (a grab-bag of grumbles). Default: script.
- `{a|b|c}` picks one variant, via a shuffle-bag (every variant dealt
  once before any repeats). Pool files get a shuffle-bag over lines the
  same way.
- `cooldown:` seconds before the bit may fire again.
- Deliberate repetition is allowed and good: catchphrases are just
  lines without variants — familiar skeleton, fresh flesh.
- Family data never reaches the public repo: `rpi/cast` (who's here
  today — see `cast.example` for the format) and `rpi/bits/private/`
  (family in-joke bits) are gitignored and live only on the Pi. The
  scanner reads subdirectories, so private bits play like any other.
  Git does not back these up — keep a private copy somewhere.

### Triggers

Bits name their trigger; the voice program defines the set — this is
the contract between code and content. Thresholds live in code, never
in bit files, so bits stay pure content. Initial set: `boot`, `idle`,
`battery_low`, `battery_critical`, `watchdog_trip`, `jolt`,
`goodnight`, `nothing_happens`. Grows as sensors land.

### Scheduler — silence is the default

The anti-tiresome machinery. A bit every ten-plus minutes feels alive;
every two minutes is a toy with no off switch.

- Event bits fire on their trigger, subject to cooldown. Safety-class
  lines (battery critical) always speak.
- Spontaneous (`idle`) bits are gated by Mike's mood: a sum of two or
  three slow sines with random phases at boot, plus impulses from real
  events that decay away. The BODY's apparent mood is derived from
  physiology — telemetry (voltage, temperature, IMU jolts). Both moods
  are computed on the Pi; the ESP has no mood, it's a part.
- A literal grumpiness knob — potentiometer on an ESP ADC, one more
  telemetry key — scales the spontaneous rate only. Facts bypass the
  knob. (Pot goes on the parts list when the knob is specced.)

### The voice program

`rpi/voice` (plain C, `make voice`). Scans `bits/` plus one level of
subdirectories at start; then:

- stdin is the event feed. A bare trigger name per line fires that
  trigger — the rehearsal seam (`printf 'goodnight\n' | ./voice -n`).
  An `ok k=v …` telemetry line derives triggers: `jolt` on a
  `moving` 0→1 edge with `thr=0`; battery triggers arm the moment
  `vbat_mv` exists in `tel`.
- `boot` fires at startup. `idle` bits self-schedule roughly every
  15 minutes, scaled by the mood clock (two slow sines, random phases
  each run) — silence stays the default.
- One eligible bit per firing, chosen at random among those off
  cooldown. Pool bits deal lines from a shuffle bag; `{a|b|c}` slots
  won't repeat their previous pick.
- Lines are spoken by exec'ing `mikesay` / `bodysay`, so the say
  scripts remain the single tuning point for pace and pitch. `-n`
  prints the performance without audio.
- Deferred deliberately: the grumpiness knob (needs the pot on an ESP
  ADC), escalation flags on repeated BODY lines, bags persisting
  across runs.

### Testing seam

`mikesay` / `bodysay` speak one line each from the command line — they
prove the audio chain the day the amp arrives. `voice -n` rehearses
whole scenes from piped trigger names or synthetic telemetry — no
rover, no port, no speaker. Port discipline is unchanged: one reader;
picocom stays the ESP-poking tool.

## Toolchains

- ESP: ESP-IDF at `~/esp/esp-idf` (a v6.1-dev checkout), target
  `esp32s3`; `esptool` in `~/venvs/esp`. Build and flash commands are in
  CLAUDE.md.
- Pi: system gcc and make.
- Voice: espeak-ng from apt; Piper in `~/venvs/piper` (pip `piper-tts`),
  voices in `~/piper-voices` (currently `en_GB-alan-medium`). Test
  tools `rpi/mikesay` / `rpi/bodysay` speak one line each, to ALSA or
  to a wav.

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

Voice:

- Piper TTS: https://github.com/rhasspy/piper (see README for the
  current maintained home) — voices: https://huggingface.co/rhasspy/piper-voices
- espeak-ng: https://github.com/espeak-ng/espeak-ng
