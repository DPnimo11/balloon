#include <SPI.h>

// Uno R3 only. With power OFF, disconnect ALL external wires from D9-D13.
// Then put one jumper directly between the Uno's D11 and D12 header pins.
// Reconnect USB power, upload this sketch, and use Serial Monitor at 115200.
// Remove the D11-D12 jumper before reconnecting any SPI modules.

const uint8_t PATTERNS[] = {0x00, 0xFF, 0x55, 0xAA, 0x3C, 0xC3};

void printHexByte(uint8_t value) {
  Serial.print(F("0x"));
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

void checkLoopback(uint32_t clockHz) {
  uint16_t mismatches = 0;
  bool firstFailure = true;
  SPI.beginTransaction(SPISettings(clockHz, MSBFIRST, SPI_MODE0));

  for (uint8_t pass = 0; pass < 20; ++pass) {
    for (uint8_t index = 0; index < sizeof(PATTERNS); ++index) {
      uint8_t expected = PATTERNS[index];
      uint8_t received = SPI.transfer(expected);
      if (received != expected) {
        ++mismatches;
        if (firstFailure) {
          Serial.print(F("First mismatch at "));
          Serial.print(clockHz);
          Serial.print(F(" Hz: sent "));
          printHexByte(expected);
          Serial.print(F(", received "));
          printHexByte(received);
          Serial.println();
          firstFailure = false;
        }
      }
    }
  }

  SPI.endTransaction();
  Serial.print(clockHz);
  Serial.print(F(" Hz loopback: "));
  if (mismatches == 0) {
    Serial.println(F("PASS (120/120 bytes matched)"));
  } else {
    Serial.print(F("FAIL ("));
    Serial.print(mismatches);
    Serial.println(F("/120 bytes mismatched)"));
  }
}

void setup() {
  digitalWrite(9, HIGH);
  pinMode(9, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(10, OUTPUT); // Uno hardware SS must remain an output.
  Serial.begin(115200);
  SPI.begin();
  delay(2000);
  Serial.println(F("Uno R3 SPI loopback: external D9-D13 wires disconnected"));
  Serial.println(F("Required jumper: directly from Uno D11 to Uno D12"));
  Serial.println(F("This tests SPI data output/input, not external SCK or CS wiring."));
}

void loop() {
  checkLoopback(125000);
  checkLoopback(1000000);
  Serial.println();
  delay(2000);
}
