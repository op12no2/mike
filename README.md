# Mike — DIY Robot

A long-lived, autonomous companion robot intended to amuse and interact with my grandchildren, indoors and outdoors.

The project will be developed iteratively. Hardware and software choices are provisional unless stated otherwise.

## Hardware

### PC

* AMD Ryzen 9 9950X.
* 64 GB RAM.
* NVIDIA RTX 5070 Ti, 16 GB VRAM.
* Runs heavier AI, language, vision, memory and planning workloads.

### Raspberry Pi

* Raspberry Pi 5, 8 GB.
* Active cooler.
* 128 GB microSD card.
* Provides local autonomy and continues operating when the PC is unavailable.

### Microcontroller

* Espressif ESP32-S3-DevKitC-1.
* Handles low-level hardware control, safety and time-critical operations.

### Mobile Platform

* Axial 1/10 SCX10 III Base Camp 4WD Rock Crawler Builder’s Kit.
* Spektrum Firma 2-in-1 brushless crawler motor/ESC, 1400Kv — probable.
* Spektrum S6295 1/8 HV high-speed, high-torque brushless metal-gear steering servo — probable.
* Wheels and tyres — TBC.
* Battery — TBC.
* Dedicated voltage regulation for the Raspberry Pi and other electronics — TBC.
* Power monitoring and protection — TBC.

## Architecture

### ESP32

Responsible for low-level, safety-critical and time-sensitive functions:

* Steering-servo control.
* Motor/ESC control.
* Emergency stop.
* Command watchdog.
* Battery voltage and current monitoring.
* Selected fast or safety-related sensors.
* Safe behaviour if communication with the Raspberry Pi is lost.

The ESP32 must fail safely and remain useful in the absence of the Raspberry Pi.

### Raspberry Pi

Responsible for local autonomy:

* Camera and audio capture.
* Wake-word detection.
* Local speech-to-text where practical.
* Sensor integration.
* Local world model.
* Navigation and obstacle avoidance.
* Behaviour selection.
* Face and object recognition.
* Short-term state and memory.
* Communication with the ESP32.
* Communication with the PC over Wi-Fi.
* Graceful operation when the PC or Wi-Fi is unavailable.

The Raspberry Pi must continue operating meaningfully in the absence of the PC.

### PC

Responsible for heavier processing:

* Larger language models.
* Richer speech-to-text and text-to-speech.
* More demanding computer-vision models.
* Long-term memory storage and retrieval.
* Reflection and memory consolidation.
* Higher-level planning.
* Development, monitoring and diagnostics.

The PC enhances Mike but should not be required for basic safe autonomous operation.

## Audio and Speech

Speech involves both hardware and software.

### Hardware

* One or more microphones.
* Possible microphone array for direction finding and noise rejection.
* Speaker.
* Audio amplifier or USB audio device if required.

### Raspberry Pi Software

* Audio-device management.
* Continuous or triggered audio capture.
* Wake-word detection.
* Voice-activity detection.
* Noise suppression and echo cancellation.
* Local speech-to-text fallback.
* Playback of locally available speech and sounds.

### PC Software

* Higher-quality speech-to-text.
* Language-model processing.
* Higher-quality text-to-speech.
* Speaker identification and conversation analysis where appropriate.

When the PC is unavailable, Mike should still be able to recognise a limited set of commands and produce useful local speech.

## Development

* Windows 11.
* WSL2.
* Visual Studio Code.
* Claude Code.
* ChatGPT and selected OpenAI APIs.
* Git and GitHub.
* C and Python on the PC and Raspberry Pi.
* C or C++ on the ESP32.
* ESP-IDF or PlatformIO — TBC.
* Serial or USB communication between Raspberry Pi and ESP32 — TBC.
* TBC.

## Sensors

### Vision and Audio

* Camera.
* Microphone or microphone array.
* Possible thermal camera.
* TBC.

### Environmental

* Ambient light.
* Ambient temperature.
* Barometric pressure.
* Humidity.
* Rain detection.
* Wind speed and possibly wind direction.
* UV level.
* Air quality.
* TBC.

### Motion and Position

* IMU: accelerometer and gyroscope.
* Wheel or drivetrain movement sensing.
* GPS.
* LiDAR.
* Short-range time-of-flight distance sensors.
* Bumpers or contact switches.
* TBC.

### Power and Internal State

* Battery voltage.
* Battery current: “I’m tired.”
* Battery temperature.
* Raspberry Pi temperature.
* Motor current.
* Servo current if useful.
* Wi-Fi signal strength.
* TBC.

### Physical Interaction

* Suspension load or pressure sensing: “Don’t sit on me.”
* Chassis tilt.
* Wheel obstruction or stall detection.
* Touch sensors.
* TBC.

## Actuators

* Drive motor.
* Steering servo.
* Speaker.
* Pan-and-tilt camera head.
* Eyes or expressive lights.
* General status lighting.
* Bird scarer.
* Arm or manipulator.
* Hand or gripper.
* TBC.

## Design Principles

* Safety around children is paramount.
* Reliability is more important than sophistication.
* Mike should degrade gracefully as systems or connectivity become unavailable.
* The PC may disappear; the Raspberry Pi should continue.
* The Raspberry Pi may fail; the ESP32 should stop movement safely.
* Character should emerge from experience, memory, internal state and behaviour rather than from hard-coded jokes or personality rules.
* Hardware and software should be modular and replaceable.
* Avoid unnecessary proprietary dependencies.
* Prefer parts with strong long-term availability and community support.
* Outdoor components must eventually account for Gower rain, wind, sand and salt air.
* Development should maximise joy rather than optimisation for its own sake.


