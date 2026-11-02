#include <SPI.h>

// Standalone Uno R3 communication test. No SD or sensor libraries required.
// Keep the SD module fully disconnected during this test.
// BME680: CS=D9, SDI/MOSI=D11, SDO/MISO=D12, SCK=D13.
// Keep the module's existing, appropriate power and ground connections.
const uint8_t BME_CS_PIN = 9;
const uint8_t SD_CS_PIN = 10;

uint8_t readRegister(uint8_t address) {
  digitalWrite(BME_CS_PIN, LOW);
  SPI.transfer(address | 0x80); // Bit 7 selects a read.
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(BME_CS_PIN, HIGH);
  return value;
}

void writeRegister(uint8_t address, uint8_t value) {
  digitalWrite(BME_CS_PIN, LOW);
  SPI.transfer(address & 0x7F);
  SPI.transfer(value);
  digitalWrite(BME_CS_PIN, HIGH);
}

uint8_t readChipId(uint32_t clockHz) {
  SPI.beginTransaction(SPISettings(clockHz, MSBFIRST, SPI_MODE0));

  // BME680 uses two SPI register pages. The chip ID at 0xD0 is on
  // the upper-address page: clear bit 4 in the page register (0xF3).
  // Select it explicitly because the previous sketch may have changed it.
  uint8_t page = readRegister(0xF3);
  writeRegister(0xF3, page & 0xEF);
  delayMicroseconds(10);
  uint8_t id = readRegister(0xD0);

  SPI.endTransaction();
  return id;
}

void printChipId(uint8_t id) {
  Serial.print(F("0x"));
  if (id < 0x10) Serial.print('0');
  Serial.print(id, HEX);
}

void setup() {
  // Preload HIGH before enabling outputs to avoid a chip-select LOW pulse.
  digitalWrite(BME_CS_PIN, HIGH);
  pinMode(BME_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(SD_CS_PIN, OUTPUT); // Uno hardware SS must remain an output.

  Serial.begin(115200);
  SPI.begin();
  delay(2000);
  Serial.println(F("BME680 SPI chip-ID test | Uno R3 | CS=D9"));
  Serial.println(F("Expected ID: 0x61. SD module must be disconnected."));
}

void loop() {
  uint8_t slowId = readChipId(125000);
  uint8_t normalId = readChipId(1000000);

  Serial.print(F("125 kHz: "));
  printChipId(slowId);
  Serial.print(F(" | 1 MHz: "));
  printChipId(normalId);

  if (slowId == 0x61 && normalId == 0x61) {
    Serial.println(F(" | BME68x chip ID responds at both speeds"));
  } else if (slowId == 0x61 || normalId == 0x61) {
    Serial.println(F(" | Inconsistent response: check connections and signal quality"));
  } else {
    Serial.println(F(" | Expected ID missing: check power, ground, CS, SDI, SDO, SCK"));
  }

  delay(1000);
}
