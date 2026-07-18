
### Electronics

- RPi 5 - https://thepihut.com/products/raspberry-pi-5?variant=42531604922563
- 128Gb SD card - https://thepihut.com/products/noobs-preinstalled-sd-card?variant=53607266091393 (current boot)
- 512Gb SSD - https://thepihut.com/products/raspberry-pi-ssd-kit-for-raspberry-pi-5?variant=53621638955393 (future boot)
- Active cooler - https://thepihut.com/products/active-cooler-for-raspberry-pi-5
- ESP32 - https://thepihut.com/products/esp32-s3-devkitc-1-development-board?variant=42212853514435

### I2C bus (STEMMA QT daisy-chain off the ESP32)

- Power monitor - Adafruit INA219 breakout, 26V ±3.2A max, STEMMA QT (addr 0x40) - £9.60 - https://thepihut.com/products/adafruit-ina219-high-side-dc-current-sensor-breakout-26v-3-2a-max
  - Battery voltage (LiPo low-voltage disarm) + electronics-rail current.
    Wire high-side in the buck branch after the Y split — motor current
    must NOT pass through it (±3.2A max).
- IMU - Adafruit LSM6DSOX 6-DoF accel + gyro, STEMMA QT (addr 0x6A) - £11.40 - https://thepihut.com/products/adafruit-lsm6dsox-6-dof-accelerometer-and-gyroscope
  - Tilt safety, and "commanded to move but nothing shaking" stall proxy.
- Status display - Adafruit Monochrome 1.3" 128x64 OLED, SSD1306, STEMMA QT (addr 0x3C) - £19.20 - https://thepihut.com/products/adafruit-monochrome-1-3-128x64-oled-graphic-display-stemma-qt-qwiic
- Cable, devkit header pins → first breakout - STEMMA QT to male jumper wires, 150mm - £1.00 - https://thepihut.com/products/stemma-qt-qwiic-jst-sh-4-pin-to-premium-male-headers-cable
- Cable, breakout → breakout ×2 - STEMMA QT 100mm - £1.00 each - https://thepihut.com/products/stemma-qt-qwiic-jst-sh-4-pin-cable-100mm-long
  - Lengths TBD once physical layout is known; 200/300/400mm variants exist.

No address conflicts; all breakouts are 3.3V with onboard pull-ups.

### Mechanical

- Chasis - Axial 1/10 SCX10 III Base Camp 4WD Rock Crawler Builders Kit - https://www.modelsport.co.uk/product/1351743
- Servo - Savox Waterproof Coreless Steel Gear Digital Servo 32kg PLUS - https://www.modelsport.co.uk/product/savox-waterproof-coreless-steel-gear-digital-servo-32kg-plus-1348759
- Motor/ESC - Hobbywing Quicrun Fusion SE for Crawler 1200kv 540 Spec - https://www.modelsport.co.uk/product/hobbywing-quicrun-fusion-se-for-crawler-1200kv-540-spec-1347890

### XT60 Power

14 AWG then 16 AWG for electronics after a Y split.

- Battery - Gens Ace LiPo Car GTech 3S 11.1V 5000mAh 60C - https://www.modelsport.co.uk/product/gens-ace-lipo-car-gtech-3s-111v-5000mah-60c-with-xt60-short-1361659
- Pololu 5V 5.5A Step-Down Voltage Regulator (D36V50F5) - https://thepihut.com/products/pololu-5v-5-5a-step-down-voltage-regulator-d36v50f5?variant=43704379015363

### Notes

- May need different pinion.
