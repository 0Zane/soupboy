<div algin="center">

# soupboy

<p>

<img src="./Pictures/ .png">
<img src="./Pictures/ .png">

</p>

### _A pipboy-like watch thats gonna be the handiest watch when you start falling out._

</div>

---

# Overview

**soupboy**

The world is falling out and even though everyone is panicking we are creating the project that gonna save the survivors in this souptastic world.

Introducing ***SOUP BOY***!!! Have your essential gadgets and information for everything thats happening around you in real-time and survive the chaos between the falling out survivors.

This project is created by Hardware Pirates who are destined to steal your *golden koi* when the world *ENDSSS......*

---

# Zine

<div align="center">

<img src="./Pictures/ X.png">

</div>

---

# BOM

| Designator | Function | Value/Part | Package | Qty | Price (USD) | Link |
|---|---|---|---|---|---|---|


*Notice: These are just recommendations that we have found to be secure and hopefully the cheapest possible option.*

---

# Features

In the period of time where the world is ending you will need this features for survival.

- **GPS Allocation** - Have access to your location anywhere
- **RF Safe Mode** - Diagnostic RF status pages with transmit actions disabled
- **Passive WiFi Scan** - Lists nearby networks for demo/status use only
- **Laser Module** - Have a decoy that will confuse other survivors of what is happening for when your laser shines.
- **Long Lasting Battery** - In a doomsday-like times you need a gadget thats going to last a long time
- **SoupBoy UI** - Boot animation, avatar, tools, status, and about screens

---

# Hardware Stack

| Subsystem | Component | Description |
|---|---|---|
| MCU | ESP32-S3-DevKitC1 | 16MB Flash, 8 MB PSRAM, Xtensa LX7 dual-core |
| RF Header | nRF24L01 module | Safe diagnostics/status placeholder; TX disabled in firmware |
| GPS | GPS Neo-6M | UART Interface for navigation |
| Display | 128x160 OLED 1.8" | SPI Interface |
| Battery | Li-Ion battery pack 2S1P 3200mAh |
| 5V Rail | LM2597 Buck Converter | Boost converter for supplying the MCU |
| Laser | ... | Built-in potentiometer for turning on a laser |

---

# Hardware Design

## Schematic

***Refer to the PDF file for better overview [PDF](Pictures/Schematic_Print.pdf)***

<img src="./Pictures/MCU_Sch.png" width="100%"> <br>

---

# Firmware

The firmware is an Arduino-style ESP32-S3 sketch in [`firmware/`](firmware/). It currently builds a polished hackathon prototype UI:

- Boot sequence with SoupBoy OS branding
- Main menu with avatar, tools, status, and about pages
- Soup avatar bitmap generated from `soup-avatar.png`
- Safe tools pages for RF, BLE, GPS, light/laser, battery, WiFi, and system info
- Passive WiFi scanning only
- RF/BLE transmit actions disabled
- GPS and RF pages fail gracefully when modules are offline

No temperature or weather-station features are implemented.

## Pin Map

Pins are centralized in [`firmware/include/pins.h`](firmware/include/pins.h).

| Function | ESP32-S3 Pin |
|---|---|
| Display RST | GPIO8 |
| Display DC | GPIO9 |
| Display CS | GPIO10 |
| Display SDA/MOSI | GPIO11 |
| Display SCL/SCK | GPIO12 |
| Button BT1 / Select | GPIO6 |
| Button BT2 / Previous | GPIO4 |
| Button BT3 / Next | GPIO5 |
| Battery divider / INT label | GPIO2 |
| GPS RX | GPIO16 |
| GPS TX | GPIO17 |
| GPS PPS / shared label | GPIO13 |
| Signal / laser control | GPIO14, held LOW by default |
| LED1 | GPIO15 |
| LED2 | GPIO18 |
| LED3 | GPIO21 |

The schematic print shows an extra `CS` label on GPIO7 and does not clearly map all NRF24L01 CE/CSN/MISO nets back to ESP32 pins. Those are left as reserved/unknown in firmware, and RF TX remains disabled.

## Build and Upload

Arduino IDE:

1. Open [`firmware/firmware.ino`](firmware/firmware.ino).
2. Select an ESP32-S3 DevKitC board.
3. Install libraries:
   - Adafruit GFX Library
   - Adafruit ST7735 and ST7789 Library
   - TinyGPSPlus
4. Compile and upload.

PlatformIO:

```bash
pio run
pio run --target upload
```

If the display colors or offsets are wrong for a specific 1.8" module, adjust `INITR_BLACKTAB` or `SCREEN_ROTATION` in the firmware.

```bash
SOUP/
|--- Firmware/
|    |--- include/
|         |--- battery.h
|    |--- battery.cpp
|--- Pictures/
|--- LICENSE
|--- README.md
|--- Soup Boy Zine.pdf
```

---

# Creators 

### Hardware Pirates: ...

---

# License

This project is licensed under the **MIT License** - see [LICENSE](LICENSE) for full terms.

---

<div align="center">

## Soup Boy

### Falling out, successfully.

</div>
