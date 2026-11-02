#ifndef BALLOON_SENSOR_PINS_H
#define BALLOON_SENSOR_PINS_H

#include <Arduino.h>

// Confirmed Uno R3 wiring. Both SPI devices use this software SPI mapping.
namespace BalloonPins {
const uint8_t BME_CS = 9;
const uint8_t SD_CS = 10;
const uint8_t BUS_MOSI = 11; // Uno -> both modules' SDI/MOSI
const uint8_t BUS_MISO = 12; // Both modules' SDO/MISO -> Uno
const uint8_t BUS_SCK = 13;
}

#endif
