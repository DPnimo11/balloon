# Project-local Arduino SD software SPI

This is the source of the installed Arduino SD **1.3.0** library, adapted for
the existing Uno R3 wiring. Upstream: https://github.com/arduino-libraries/SD.
Original copyright notices and `LICENSE.txt` are retained.

Local changes:

- `utility/Sd2Card.h` enables the library's existing software SPI path and
  reads its pins from `src/SensorPins.h`. The driver targets ATmega328P.
- `utility/Sd2Card.cpp` leaves hardware SPI disabled and adds 2 microsecond
  half-period delays to the software transfers. The resulting clock stays
  below the SD initialization limit of 400 kHz.
- `File.cpp` uses a quoted local header include so the installed SD library
  cannot be selected accidentally.
- `SD.cpp` skips the hardware-only clock setter in the optional clock overload.
  Software SPI always uses the conservative transfer timing described above.

The sketch includes this copy directly, and Arduino IDE compiles it recursively
under `src`. No modifications to installed libraries are required. Keep the
entire `src` folder with the main `.ino` when moving or sharing the sketch.
