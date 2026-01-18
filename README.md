# Teensy2DMD - T4.1 Edition with Clock & EEPROM

This is an enhanced version of the **Teensy2DMD** port for Teensy 4.1 and SmartLED Shield V5. This specific version adds a real-time clock (RTC) feature and persistent memory settings.

## 🕒 New Features (Clock Edition)
* **Real-Time Clock (RTC)**: Integrated a clock display that can be toggled and updated via serial commands.
* **EEPROM Memory**: Your settings (like brightness or clock display) are now saved automatically. When you restart the Teensy, it remembers exactly where it left off.
* **Hardware**: Specifically optimized for **Teensy 4.1** and **SmartLED Shield V5 (HUB75)**.

## 🚀 Improvements from previous version
* **Boot Screen**: Custom "*** RETROPIE ARCADE SYSTEM READY ***" startup message.
* **New Commands**: 
    * `ON` / `OFF`: To control the display power.
    * `SETTIME`: To update the internal clock via serial.
* **Performance**: Optimized for high-speed GIF playback on the Teensy 4.1.

## 💾 Easy Install (HEX File)
1. Download the included `.hex` file.
2. Use the **Teensy Loader** to flash it directly to your Teensy 4.1.

## ⚙️ Requirements for Developers
* **Arduino IDE**: 1.8.19 recommended.
* **Teensyduino**: Version 1.58 or higher.
* **Libraries**: 
    * SmartMatrix 4.0.3
    * SdFat 2.3.0
    * TimeLib (for the clock)

## 📂 How to upload GIFs
1. Connect your Teensy to your PC.
2. Open a terminal (like Tera Term) at 57600 baud.
3. Type `RZ` and use **ZMODEM** to send your `.gif` files to the SD card.

---
*Credits to the original creator gi1mic and the previous T4.1 port contributors.*