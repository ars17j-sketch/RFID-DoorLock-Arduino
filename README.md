# RFID Door Lock Arduino UNO

## Overview

RFID Door Lock adalah sistem keamanan pintu berbasis **Arduino Uno** yang menggunakan **RFID MFRC522** sebagai autentikasi utama dan **Keypad 4x4** sebagai PIN cadangan.

Project ini dirancang agar stabil, mudah digunakan, dan mudah dikembangkan untuk kebutuhan rumah, kantor, laboratorium, maupun tugas akhir.

---

## Features

* RFID MFRC522
* Master Card
* Admin Mode
* Add RFID Card
* Delete RFID Card
* EEPROM Storage
* PIN Backup (4 Digit)
* Relay Door Lock Control
* LCD I2C 16x2 Display
* Buzzer Notification
* Anti RFID Hang
* PIN Timeout
* PIN Lockout Protection
* LCD Flicker Reduction
* Stable RFID Reading

---

## Hardware

| Component              | Quantity |
| ---------------------- | -------: |
| Arduino Uno R3         |        1 |
| MFRC522 RFID Module    |        1 |
| RFID Card / Tag        |       1+ |
| LCD I2C 16x2           |        1 |
| Relay Module 5V        |        1 |
| Solenoid Door Lock 12V |        1 |
| Keypad 4x4             |        1 |
| Active Buzzer          |        1 |
| Power Supply 12V       |        1 |

---

## Pin Configuration

### RFID

| MFRC522 | Arduino UNO |
| ------- | ----------- |
| SDA     | D10         |
| SCK     | D13         |
| MOSI    | D11         |
| MISO    | D12         |
| RST     | A0          |
| GND     | GND         |
| 3.3V    | 3.3V        |

---

### LCD I2C

| LCD | Arduino |
| --- | ------- |
| SDA | A4      |
| SCL | A5      |
| VCC | 5V      |
| GND | GND     |

---

### Relay

| Relay | Arduino |
| ----- | ------- |
| IN    | A1      |
| VCC   | 5V      |
| GND   | GND     |

---

### Buzzer

| Buzzer | Arduino |
| ------ | ------- |
| +      | A2      |
| -      | GND     |

---

### Keypad

| Keypad | Arduino |
| ------ | ------- |
| R1     | D9      |
| R2     | D8      |
| R3     | D7      |
| R4     | D6      |
| C1     | D5      |
| C2     | D4      |
| C3     | D3      |
| C4     | D2      |

---

Adaptor 12V (+)
      │
      │
      ├──────── COM Relay
      │
      └──────── (+) Door Lock

NO Relay ─────── (+) Door Lock

(-) Door Lock ── (-) Adaptor 12V



## Required Libraries

Install the following libraries from Arduino IDE Library Manager:

* MFRC522
* LiquidCrystal_I2C
* Keypad
* EEPROM (Built-in)
* SPI (Built-in)

---

## Project Structure

```text
RFID-DoorLock-Arduino
│
├── RFID_DoorLock_Final.ino
├── README.md
├── LICENSE
└── images
```

---

## How To Use

1. Upload `RFID_DoorLock_Final.ino`.
2. Scan the Master Card.
3. Enter Admin Mode.
4. Add or remove RFID cards.
5. Scan registered cards to unlock the door.
6. If RFID is unavailable, use the backup PIN.

---

## Security Features

* Master Card Authentication
* EEPROM Card Database
* Backup PIN
* Wrong PIN Lockout
* RFID Auto Recovery
* Stable Relay Control

---

## Author

Created by **Budi**

Developed using Arduino IDE.

---

## License

This project is released under the MIT License.
