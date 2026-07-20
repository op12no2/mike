# Mike

A grumpy companion rover for my grandchildren.

Mike is an Axial SCX10 III crawler chassis carrying a Raspberry Pi 5 (the
brain: speech, vision, persona — eventually) and an ESP32-S3 (the body:
PWM, sensors, and a watchdog that stops the wheels when the brain goes
quiet). One USB cable joins them, carrying power, a line protocol, and
field reflashing. He is slow and torquey by design, Marvin-adjacent by
temperament — reluctant, put-upon, secretly devoted — and as beach-proof
as we can manage.

The goal is theatre, not utility: he detects the children, trundles over,
and complains about it.

## Reading order

- [protocol.md](protocol.md) — the wire protocol between brain and body.
- [hardware.md](hardware.md) — the electrical design, and why.
- [software.md](software.md) — the software shape and toolchains.
- [parts.md](parts.md) — the shopping list.
- [CLAUDE.md](CLAUDE.md) — working notes, doctrine, current status.

Everything is plain C, flat files, and Makefiles. Unknown serial commands
are answered with `Nothing happens.`
