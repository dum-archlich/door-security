/***************************************************
  ESP32 + R308 FINGERPRINT ENROLL
  - Menggunakan HardwareSerial2 (GPIO16/17)
  - Baud rate sensor bisa diubah di FP_BAUD
 ****************************************************/

#include <Adafruit_Fingerprint.h>

// --- KONFIGURASI PIN UNTUK ESP32 ---
// RX2 pada ESP32 (GPIO 16) terhubung ke TX (Kabel Kuning) Sensor
// TX2 pada ESP32 (GPIO 17) terhubung ke RX (Kabel Hijau) Sensor
#define RXD2 16
#define TXD2 17

// --- UBAH INI JIKA MAU COBA BAUD RATE LAIN ---
// Coba: 57600, 9600, 19200, 115200
#define FP_BAUD 9600

// Inisialisasi Serial2
HardwareSerial mySerial(2);

// Buat objek sensor fingerprint menggunakan Serial2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

// Deklarasi fungsi
bool getFingerprintEnroll();
uint8_t readnumber(void);

void setup() {
  // Serial Monitor
  Serial.begin(9600);
  delay(500);

  Serial.println();
  Serial.println("=== ESP32 + R308 Fingerprint Enroll ===");
  Serial.print("Menggunakan baud rate sensor: ");
  Serial.println(FP_BAUD);

  // Memulai Serial2 untuk komunikasi dengan Sensor
  mySerial.begin(FP_BAUD, SERIAL_8N1, RXD2, TXD2);

  // Inisialisasi sensor fingerprint
  finger.begin(FP_BAUD);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    Serial.println("Cek sambungan kabel RX/TX Anda (Pin 16 & 17).");
    Serial.println("Pastikan:");
    Serial.println("- VCC -> 5V");
    Serial.println("- GND -> GND");
    Serial.println("- KUNING (TX sensor) -> GPIO16 (RX2)");
    Serial.println("- HIJAU (RX sensor)  -> GPIO17 (TX2)");
    while (1) {
      delay(1); // berhenti di sini kalau sensor tidak ketemu
    }
  }

  Serial.println(F("Reading sensor parameters..."));
  finger.getParameters();
  Serial.println("Sensor siap.\n");
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    // Menunggu input dari pengguna di Serial Monitor
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop() {
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");

  id = readnumber(); // Program berhenti di sini menunggu Anda mengetik angka ID

  if (id == 0) {
    // ID #0 tidak boleh, ulangi lagi
    Serial.println("ID 0 tidak diperbolehkan, silakan masukkan ID 1-127.");
    return;
  }

  Serial.print("Enrolling ID #");
  Serial.println(id);

  // Ulangi proses enroll sampai berhasil
  while (!getFingerprintEnroll()) {
    Serial.println("Terjadi kesalahan saat pendaftaran jari. Mengulang proses...");
    Serial.println();
    delay(2000);
  }

  // Jika sampai sini, berarti sukses
  Serial.println("Pendaftaran jari selesai. Anda bisa mendaftarkan ID lain.\n");
}

bool getFingerprintEnroll() {
  int p = -1;

  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);

  // ---- LANGKAH 1: Ambil gambar pertama ----
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        // diam saja biar serial tidak penuh
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return false;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return false;
      default:
        Serial.println("Unknown error (getImage #1)");
        return false;
    }
  }

  // ---- LANGKAH 2: Konversi gambar pertama ke template buffer 1 ----
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted (1)");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error (image2Tz #1)");
      return false;
  }

  // ---- LANGKAH 3: Minta jari diangkat ----
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  // ---- LANGKAH 4: Minta jari yang sama ditempel lagi ----
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        // diam saja
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return false;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return false;
      default:
        Serial.println("Unknown error (getImage #2)");
        return false;
    }
  }

  // ---- LANGKAH 5: Konversi gambar kedua ke template buffer 2 ----
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted (2)");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error (image2Tz #2)");
      return false;
  }

  // ---- LANGKAH 6: Buat model dari 2 template ----
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return false;
  } else {
    Serial.println("Unknown error (createModel)");
    return false;
  }

  // ---- LANGKAH 7: Simpan model ke flash sensor ----
  Serial.print("Storing model at ID ");
  Serial.println(id);

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("\n---");
    Serial.println("Stored!");
    Serial.println("SUKSES! Jari berhasil didaftarkan.");
    Serial.println("Silakan masukkan ID baru untuk mendaftarkan jari lain.");
    Serial.println("---\n");
    return true;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return false;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return false;
  } else {
    Serial.println("Unknown error (storeModel)");
    return false;
  }
}
