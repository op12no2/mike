# Mike body protocol — version 2

The wire between the brain (Pi 5) and the body (ESP32-S3): UART0 over the
devkit USB cable, `/dev/ttyUSB0`, 115200 8N1. This file is the single
source of truth; firmware and Pi-side code follow it.

## Line rules

- ASCII. A command is one line, terminated `\n`; a stray `\r` is ignored.
- All numbers on the wire are decimal integers. No floats, ever.
- Max 120 bytes per line. Overlong input is discarded up to the next
  newline and answered `eh?`.
- Empty lines are ignored (no reply).
- Strict request/response: one command, exactly one reply line, one
  command in flight at a time. The body never speaks unsolicited.
- The brain must ignore any line it cannot parse as a reply — this
  absorbs boot chatter while UART0 is still shared with the console.

## Replies

| reply | meaning |
|---|---|
| `ok [data]` | done; `data` is command-specific |
| `pong` | you pinged |
| `mike <v>` | protocol version |
| `eh?` | known verb, malformed args (wrong count, not a number, out of range) |
| `no <reason>` | understood but refused — e.g. `no disarmed` |
| `Nothing happens.` | unknown verb |

## Commands

Argument counts are strict: too many or too few is `eh?`.

| command | reply | notes |
|---|---|---|
| `ping` | `pong` | link check |
| `ver` | `mike 2` | checked by the brain at connect |
| `arm` | `ok` | enables motion, starts the throttle lease |
| `disarm` | `ok` | throttle neutral, lease off; the resting state |
| `drv <thr> <str>` | `ok <thr> <str>` | −1000…1000 each; `no disarmed` when disarmed; reply echoes the values actually applied after clamping |
| `stop` | `ok` | throttle neutral now, stays armed, refreshes the lease |
| `led <r> <g> <b>` | `ok` | onboard RGB, 0…255 each |
| `tel` | `ok k=v k=v …` | body state; pure state echo, touches no sensor |
| `imu` | `ok k=v k=v …` | one LSM6DSOX read |

One query per sensor, deliberately: the brain polls each at its own
rate (a rate is brain policy, not protocol), and a slow or absent
sensor never delays the others. A new sensor is a new query verb —
planned: `pwr` (INA219: `vbat_mv`, `ibat_ma`).

## Safety model

**Throttle is a lease.** While armed, a valid `drv` (or `stop`) must
arrive at least every **300 ms**; if it doesn't, the body sets throttle
to neutral, holds steering where it is, disarms, and increments
`wdtrips`. Re-arming is an explicit `arm` — the brain must acknowledge
that the rover stopped underneath it. While armed the brain streams
`drv` at ~10 Hz even when the values are unchanged (`drv 0 0` when
stationary but armed).

- Telemetry queries (`tel`, `imu`, …) do **not** feed the lease: a live
  poller with a dead drive loop must not keep the wheels leased.
- Boot state: disarmed, throttle neutral. Once PWM exists, neutral is
  emitted continuously from before the first command is accepted, and a
  lease trip keeps *emitting* neutral rather than dropping the signal —
  the ESC's own signal-loss failsafe stays a backup layer, not the
  mechanism.
- Firmware hang: the ESP task watchdog resets the chip, which reboots
  into the disarmed-neutral state. Every failure degrades to stillness.

## Units and clamps

`thr` and `str` are abstract −1000…1000 (sign conventions get fixed when
the wheels are wired). The body owns the mapping to servo/ESC
microseconds — center, endpoints, direction are calibration constants in
firmware — and clamps throttle to its soft limit (currently ±400)
regardless of what the brain asks. Slow and torquey is enforced in the
body, where the brain can't talk it out of it. The `drv` reply echoes
applied values so clamping is visible.

## Telemetry keys

Telemetry replies are `ok` followed by space-separated `key=value`
pairs, all integers. The brain must ignore keys it doesn't know — new
keys arrive without a version bump.

`tel` — body state:

| key | unit | meaning |
|---|---|---|
| `up_ms` | ms | body uptime |
| `armed` | 0/1 | motion enabled |
| `wdtrips` | count | lease expiries since boot |
| `thr` | −1000…1000 | applied throttle |
| `str` | −1000…1000 | applied steering |

`imu` — LSM6DSOX:

| key | unit | meaning |
|---|---|---|
| `imu` | 0/1 | present and readable this poll; when 0 the keys below are omitted |
| `pitch_mdeg` | millidegrees | static tilt from the accelerometer; axis orientation and signs get fixed at mounting |
| `roll_mdeg` | millidegrees | static tilt from the accelerometer |
| `moving` | 0/1 | rotating faster than ~5°/s on any axis right now (instantaneous, not latched) |

Planned: `pwr` with `vbat_mv`, `ibat_ma` (INA219).

## Versioning

`ver` returns the protocol version. Adding commands or telemetry keys is
not a bump; changing the meaning or shape of anything existing is. The
brain checks `ver` at connect and refuses to drive a body it doesn't
understand.

History: v1 folded the IMU keys into `tel`; v2 split telemetry into
per-sensor queries so each can be polled at its own rate.

## Port ownership

One reader at a time (see CLAUDE.md). The brain opens `/dev/ttyUSB0`
once and keeps it open — open/close cycles can reset the body via DTR —
and clears HUPCL so exiting doesn't drop DTR. Flashing needs the port
closed.
