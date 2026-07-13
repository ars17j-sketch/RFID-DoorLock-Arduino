```cpp
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Keypad.h>

/* ===================== PIN CONFIG ===================== */
#define SS_PIN      10
#define RST_PIN     A0
#define RELAY_PIN   A1      // ACTIVE LOW relay
#define BUZZER_PIN  A2

/* ===================== MODULES ===================== */
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* ===================== MASTER CARD ===================== */
byte MASTER_UID[4] = {0x83, 0x43, 0x83, 0x29}; // ganti sesuai master card Anda

/* ===================== EEPROM MAP ===================== */
#define EEPROM_COUNT_ADDR  0
#define EEPROM_DATA_ADDR   10
#define MAX_CARD           10

/* ===================== KEYPAD ===================== */
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9,8,7,6};
byte colPins[COLS] = {5,4,3,2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/* ===================== PIN BACKUP ===================== */
const char PIN_CODE[] = "1234";
char pinInput[5] = {0};
byte pinIndex = 0;

/* ===================== STATE ===================== */
byte jumlahKartu = 0;
bool adminMode = false;

bool doorOpen = false;
unsigned long doorCloseAt = 0;

bool pinEntryActive = false;
unsigned long pinLastKeyAt = 0;
const unsigned long PIN_TIMEOUT_MS = 10000;

byte wrongPinCount = 0;
unsigned long lockoutUntil = 0;
const unsigned long LOCKOUT_MS = 30000;
const byte MAX_WRONG_PIN = 3;

String lcdLine0 = "";
String lcdLine1 = "";

/* ===================== UTIL: LCD ===================== */
void lcdShow(const String &l1, const String &l2 = "") {
  if (l1 == lcdLine0 && l2 == lcdLine1) return;
  lcdLine0 = l1;
  lcdLine1 = l2;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

/* ===================== UTIL: BUZZER ===================== */
void beepOn(unsigned int ms) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(ms);
  digitalWrite(BUZZER_PIN, LOW);
}

void bunyiOK() {
  beepOn(120);
}

void bunyiKlik() {
  beepOn(30);
}

void bunyiTolak() {
  for (byte i = 0; i < 3; i++) {
    beepOn(80);
    delay(80);
  }
}

/* ===================== EEPROM ===================== */
byte bacaJumlahKartu() {
  byte n = EEPROM.read(EEPROM_COUNT_ADDR);
  if (n > MAX_CARD) n = 0;
  return n;
}

void simpanJumlahKartu(byte n) {
  EEPROM.update(EEPROM_COUNT_ADDR, n);
}

bool kartuSamaDenganEEPROM(byte *uid, byte indexSlot) {
  for (byte i = 0; i < 4; i++) {
    if (uid[i] != EEPROM.read(EEPROM_DATA_ADDR + (indexSlot * 4) + i)) return false;
  }
  return true;
}

bool cekKartuTerdaftar(byte *uid) {
  for (byte i = 0; i < jumlahKartu; i++) {
    if (kartuSamaDenganEEPROM(uid, i)) return true;
  }
  return false;
}

bool cekKartuMaster(byte *uid) {
  for (byte i = 0; i < 4; i++) {
    if (uid[i] != MASTER_UID[i]) return false;
  }
  return true;
}

void simpanKartuBaru(byte *uid) {
  if (jumlahKartu >= MAX_CARD) {
    lcdShow("MEMORI PENUH", "Kartu ditolak");
    bunyiTolak();
    return;
  }

  for (byte i = 0; i < 4; i++) {
    EEPROM.update(EEPROM_DATA_ADDR + (jumlahKartu * 4) + i, uid[i]);
  }

  jumlahKartu++;
  simpanJumlahKartu(jumlahKartu);

  lcdShow("KARTU DITAMBAH", "Berhasil");
  bunyiOK();
}

void hapusKartu(byte *uid) {
  for (byte i = 0; i < jumlahKartu; i++) {
    if (kartuSamaDenganEEPROM(uid, i)) {
      for (byte k = i; k < jumlahKartu - 1; k++) {
        for (byte j = 0; j < 4; j++) {
          EEPROM.update(
            EEPROM_DATA_ADDR + (k * 4) + j,
            EEPROM.read(EEPROM_DATA_ADDR + ((k + 1) * 4) + j)
          );
        }
      }

      byte lastSlot = jumlahKartu - 1;
      for (byte j = 0; j < 4; j++) {
        EEPROM.update(EEPROM_DATA_ADDR + (lastSlot * 4) + j, 0xFF);
      }

      jumlahKartu--;
      simpanJumlahKartu(jumlahKartu);

      lcdShow("KARTU DIHAPUS", "Berhasil");
      bunyiOK();
      return;
    }
  }

  lcdShow("KARTU TIDAK", "TERDAFTAR");
  bunyiTolak();
}

/* ===================== RFID CONTROL ===================== */
void resetRFID() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void tampilModeStandby() {
  if (adminMode) {
    lcdShow("ADMIN MODE", "Tempel kartu");
  } else {
    lcdShow("TEMPEL / PIN", "Siap digunakan");
  }
}

void bukaPintu(const char *msg) {
  digitalWrite(RELAY_PIN, LOW);  // active low = unlock
  doorOpen = true;
  doorCloseAt = millis() + 3000;
  lcdShow(msg, "PINTU TERBUKA");
  bunyiOK();
}

void tutupPintu() {
  digitalWrite(RELAY_PIN, HIGH);
  doorOpen = false;
  doorCloseAt = 0;
  tampilModeStandby();
}

/* ===================== PIN CONTROL ===================== */
void resetPINInput() {
  memset(pinInput, 0, sizeof(pinInput));
  pinIndex = 0;
  pinEntryActive = false;
}

void prosesPIN(char key) {
  if (lockoutUntil > millis()) {
    long sisa = (long)(lockoutUntil - millis()) / 1000;
    lcdShow("PIN LOCKED", String("Tunggu ") + sisa + "s");
    bunyiTolak();
    return;
  }

  if (key == '*') {
    resetPINInput();
    lcdShow("PIN DIBATALKAN", "Siap lagi");
    bunyiKlik();
    return;
  }

  if (key == '#') {
    pinInput[pinIndex] = '\0';
    if (strcmp(pinInput, PIN_CODE) == 0) {
      wrongPinCount = 0;
      resetPINInput();
      bukaPintu("PIN BENAR");
    } else {
      wrongPinCount++;
      lcdShow("PIN SALAH", String("Percobaan ") + wrongPinCount + "/" + MAX_WRONG_PIN);
      bunyiTolak();
      if (wrongPinCount >= MAX_WRONG_PIN) {
        lockoutUntil = millis() + LOCKOUT_MS;
        wrongPinCount = 0;
        lcdShow("PIN DIBLOKIR", "Tunggu 30 detik");
      }
      resetPINInput();
    }
    return;
  }

  if (pinIndex < 4) {
    pinInput[pinIndex++] = key;
    pinEntryActive = true;
    pinLastKeyAt = millis();
    lcd.setCursor(pinIndex - 1, 1);
    lcd.print('*');
  }
}

/* ===================== RFID PROCESS ===================== */
bool bacaRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  byte uid[4];
  if (rfid.uid.size < 4) {
    resetRFID();
    return true;
  }

  for (byte i = 0; i < 4; i++) uid[i] = rfid.uid.uidByte[i];

  if (cekKartuMaster(uid)) {
    adminMode = !adminMode;
    if (adminMode) lcdShow("ADMIN MODE", "Aktif");
    else lcdShow("MODE NORMAL", "Aktif");
    bunyiOK();
    delay(300);
    resetRFID();
    tampilModeStandby();
    return true;
  }

  if (adminMode) {
    if (cekKartuTerdaftar(uid)) hapusKartu(uid);
    else simpanKartuBaru(uid);
    delay(400);
    resetRFID();
    tampilModeStandby();
    return true;
  }

  if (cekKartuTerdaftar(uid)) {
    bukaPintu("AKSES DITERIMA");
  } else {
    lcdShow("AKSES DITOLAK", "RFID tidak valid");
    bunyiTolak();
  }

  delay(250);
  resetRFID();
  tampilModeStandby();
  return true;
}

/* ===================== KEYPAD ===================== */
void bacaKeypad() {
  if (lockoutUntil > millis()) return;

  char key = keypad.getKey();
  if (!key) return;

  bunyiKlik();
  pinLastKeyAt = millis();

  if (key >= '0' && key <= '9') {
    if (!pinEntryActive) {
      resetPINInput();
      lcdShow("INPUT PIN", "****");
      pinEntryActive = true;
    }
    if (pinIndex < 4) {
      pinInput[pinIndex++] = key;
      lcd.setCursor(pinIndex - 1, 1);
      lcd.print('*');
    }
    return;
  }

  if (pinEntryActive && (key == '*' || key == '#')) {
    prosesPIN(key);
    return;
  }

  if (key == 'A') {
    lcdShow("MODE ADMIN", "Scan master card");
    return;
  }
}

/* ===================== SETUP ===================== */
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // terkunci
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  SPI.begin();
  rfid.PCD_Init();

  jumlahKartu = bacaJumlahKartu();
  if (jumlahKartu > MAX_CARD) jumlahKartu = 0;

  lcdShow("PINTU RFID+PIN", "Starting...");
  bunyiOK();
  delay(500);
  tampilModeStandby();
}

/* ===================== LOOP ===================== */
void loop() {
  unsigned long now = millis();

  if (doorOpen && doorCloseAt != 0 && now >= doorCloseAt) {
    tutupPintu();
  }

  if (pinEntryActive && (now - pinLastKeyAt > PIN_TIMEOUT_MS) && pinIndex > 0) {
    resetPINInput();
    lcdShow("PIN TIMEOUT", "Masukkan lagi");
  }

  if (lockoutUntil > 0 && now >= lockoutUntil) {
    lockoutUntil = 0;
    tampilModeStandby();
  }

  bacaRFID();
  bacaKeypad();
}
```

