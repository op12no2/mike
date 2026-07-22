What to buy and where from. How it's wired is `hardware.md`.

### Electronics

- RPi 5 - https://thepihut.com/products/raspberry-pi-5?variant=42531604922563
- 128Gb SD card - https://thepihut.com/products/noobs-preinstalled-sd-card?variant=53607266091393 (current boot)
- 512Gb SSD - https://thepihut.com/products/raspberry-pi-ssd-kit-for-raspberry-pi-5?variant=53621638955393 (future boot)
- Active cooler - https://thepihut.com/products/active-cooler-for-raspberry-pi-5
- ESP32 - https://thepihut.com/products/esp32-s3-devkitc-1-development-board?variant=42212853514435

### I2C breakouts (STEMMA QT)

- Power monitor - Adafruit INA219 breakout, 26V ±3.2A max, STEMMA QT - £9.60 - https://thepihut.com/products/adafruit-ina219-high-side-dc-current-sensor-breakout-26v-3-2a-max
- IMU - Adafruit LSM6DSOX 6-DoF accel + gyro, STEMMA QT - £11.40 - https://thepihut.com/products/adafruit-lsm6dsox-6-dof-accelerometer-and-gyroscope
- Status display - Adafruit Monochrome 1.3" 128x64 OLED, SSD1306, STEMMA QT - £19.20 - https://thepihut.com/products/adafruit-monochrome-1-3-128x64-oled-graphic-display-stemma-qt-qwiic
- Cable, devkit header pins → first breakout - STEMMA QT to male jumper wires, 150mm - £1.00 - https://thepihut.com/products/stemma-qt-qwiic-jst-sh-4-pin-to-premium-male-headers-cable
- Cable, breakout → breakout ×2 - STEMMA QT 100mm - £1.00 each - https://thepihut.com/products/stemma-qt-qwiic-jst-sh-4-pin-cable-100mm-long
  - Lengths TBD once physical layout is known; 200/300/400mm variants exist.

### Audio

- Amp - Adafruit I2S 3W Class D Amplifier Breakout MAX98357A - £5.70 - https://thepihut.com/products/adafruit-i2s-3w-class-d-amplifier-breakout-max98357a
- Speaker - 3" 4Ω 3W bare cone - £2.90 - https://thepihut.com/products/speaker-3-diameter-4-ohm-3-watt
  - Bench/first-shell part; weather-resistant replacement TBD at shell
    layout (see hardware.md, Audio).

### Power

- Battery - Gens Ace LiPo Car GTech 3S 11.1V 5000mAh 60C - https://www.modelsport.co.uk/product/gens-ace-lipo-car-gtech-3s-111v-5000mah-60c-with-xt60-short-1361659
- Charger - Gens Ace Imars S100 (auto-recognises GTech packs; has the
  storage mode the session-end ritual relies on) - in hand
- LiPo charging bag - in hand
- Regulator - Pololu 5V 5.5A Step-Down Voltage Regulator (D36V50F5) - https://thepihut.com/products/pololu-5v-5-5a-step-down-voltage-regulator-d36v50f5?variant=43704379015363
- On order: Amass XT60 pairs, inline mini blade fuse holder + 5A fuses,
  14 and 16 AWG silicone wire, adhesive-lined heat shrink; Hobbywing LED
  Program Card (30501003) — sets ESC LiPo cutoff, drag brake, BEC
  voltage; perfboard and 2.54mm male pin headers for the junction (see
  hardware.md).
- To order: thin signal wire (22–26 AWG, stranded), junction to ESP.

### Mechanical

- Chassis - Axial 1/10 SCX10 III Base Camp 4WD Rock Crawler Builders Kit - https://www.modelsport.co.uk/product/1351743
- Servo - Savox SW-1210SG+ Waterproof Coreless Steel Gear Digital Servo 32kg PLUS - https://www.modelsport.co.uk/product/savox-waterproof-coreless-steel-gear-digital-servo-32kg-plus-1348759
- Motor/ESC - Hobbywing Quicrun Fusion SE for Crawler 1200kv 540 Spec - https://www.modelsport.co.uk/product/hobbywing-quicrun-fusion-se-for-crawler-1200kv-540-spec-1347890

### Notes

- TBD at shell layout: chunky momentary "goodnight button" (see CLAUDE.md,
  session end).
- Running total: ~£1066 (2026-07-22).
