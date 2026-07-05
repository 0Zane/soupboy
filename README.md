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
- **RF Jamming System** - Alter others connections ensuring dominance over the digital enemies around you
- **Laser Module** - Have a decoy that will confuse other survivors of what is happening for when your laser shines.
- **Long Lasting Battery** - In a doomsday-like times you need a gadget thats going to last a long time
- **Weather Station** - Have an overview on what is the current state of the weather 

---

# Hardware Stack

| Subsystem | Component | Description |
|---|---|---|
| MCU | ESP32-S3-DevKitC1 | 16MB Flash, 8 MB PSRAM, Xtensa LX7 dual-core |
| Sensors | ... | Access to weather information |
| Signal Jammer | nRF24L01 module | Frequency emitter |
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

#Firmware

Firmware features:
- ...

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
