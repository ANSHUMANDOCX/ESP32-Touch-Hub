# ESP32-S3 Smart Hub

## Description
This is a ESP32-S3 Based HUB paired with a touch screen that can be used for anything that one wants but for me i will be using it to connect all my ESP32 project and control it from one place and get a whole lot more connected experience between my project.

---
## Parts Used
- ESP32 S3 Used as the brains of the system
- CH340C for onboard programming. 
- 3.5" ILI9488 SPI touch display (MSP3520 module) for UI interaction
- Battery-powered with USB-C charging and intelligent load sharing
- Onboard SD card for storing web assets, UI resources, and sensor logs
- 3 tactile buttons + passive buzzer for physical interaction and alerts
- Charging status indicators both physical and UI based
---
## Technicals 
- The board will have dual volages 5V and 3.3v for the TFT and ESP32.
- It uses a load sharing circuit where when the battery is charging the board will be powered from the USB
- I will pair a 1800mah Lipo Battery so i can get a considerable amount of battery life.
- the board is using a MT3608 and ME6217C33M5G ics for 5v and 3.3v regulator along with a MCP73833 for charging the battery

## Firmware
*ITS YET TO BE TESTED*
## PCB SCHEMATIC and CAD 
<img width="1162" height="811" alt="image" src="https://github.com/user-attachments/assets/1adae1c7-8d83-4371-a157-f9d72f61ec40" />
<img width="1393" height="804" alt="Screenshot 2026-04-26 230145" src="https://github.com/user-attachments/assets/5adb140a-79d5-40dd-add8-98056622ea33" />
<img width="640" height="421" alt="Screenshot 2026-04-26 224944" src="https://github.com/user-attachments/assets/7fbe173e-a6e0-48bd-950b-729372326b9b" />
<img width="697" height="429" alt="Screenshot 2026-04-26 224915" src="https://github.com/user-attachments/assets/0cfea25d-e1e3-4189-88f2-14b26c33325e" />
<img width="939" height="774" alt="Screenshot 2026-04-27 014422" src="https://github.com/user-attachments/assets/46659d56-20cc-4dac-aa74-398fef26e33b" />

## BOM
| Product Name | Quantity | Price ($) | Vendor |
|---|---:|---:|---|
| [ME6217C33M5G 3.3V LDO Regulator](https://sharvielectronics.com/product/me6217c33m5g-3-3v-800ma-fixed-ldo-voltage-regulator-sot-23-5-package/) | 1 | 0.30 | Sharvi Electronics |
| [ESP32-S3-WROOM-1 N16R8](https://sharvielectronics.com/product/esp32-s3-wroom-1-n16r8-16mb-flash-memory-wifi-and-bluetooth-module-41-smd-package/) | 1 | 4.51 | Sharvi Electronics |
| [FS8205A Dual N-Channel MOSFET](https://sharvielectronics.com/product/fs8205a-dual-n-channel-power-mosfet-lithium-battery-protection-tssop-8-package/) | 1 | 0.80 | Sharvi Electronics |
| [DW01A Battery Protection IC](https://sharvielectronics.com/product/dw01a-lithium-battery-protection-ic-sot23-6/) | 1 | 0.55 | Sharvi Electronics |
| [MT3608 Step-Up Converter IC](https://sharvielectronics.com/product/mt3608-high-efficiency-1-2mhz-2a-step-up-converter-sot23-6-package/) | 1 | 0.85 | Sharvi Electronics |
| [Micro SD Card Socket 9-Pin SMD](https://sharvielectronics.com/product/micro-sd-memory-card-socket-9pin-smd/) | 1 |0.60| Sharvi Electronics |
| [MCP73833 Li-Ion Charger IC](https://robu.in/product/mcp73833-ami-un-microchip-battery-charger-for-1-cell-of-li-ion-li-pol-battery-6v-input-4-2v-1a-charge-msop-10) | 1 | 1.93 | Robu.in |
| [AO3401 P-Channel MOSFET](https://robu.in/product/ao3401a-umwyoutai-semiconductor-co-ltd-30v-4-2a-50m%cf%8910v-1-4w-400mv-1-piece-p-channel-sot-23-mosfets-rohs) | 1 | 0.37 | Robu.in |
| [3.5" TFT Touchscreen Display](https://techiesms.com/product/3-5-inch-tft-touchscreen-display/) | 1 | 11.62 | Techiesms |
| Passives (Resistors, Capacitors, Pull-ups, etc.) | Assorted | 8 | Sharvi Electronics |
|PCB||2|ALLPCB|
|SHIPPING||1||
|Total||35||
