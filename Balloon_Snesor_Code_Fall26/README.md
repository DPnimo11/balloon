# Balloon sensor logger — Uno R3

Open `Balloon_Snesor_Code_Fall26.ino`, select **Arduino Uno**, upload, and open
Serial Monitor at **115200 baud**. Keep the `src` folder beside the `.ino`.

The sketch matches the existing physical wiring using software SPI:

| Connection | Uno R3 pin |
| --- | --- |
| BME680 CS | D9 |
| SD card CS | D10 |
| Shared SDI/MOSI (Uno to modules) | D11 |
| Shared SDO/MISO (modules to Uno) | D12 |
| Shared SCK | D13 |
| UV analog output | A0 |
| MPU6050 SDA / SCL | A4 / A5 |

The bus and chip-select pins are defined once in `src/SensorPins.h`.
Keep the two chip-select wires separate. Each data/clock signal is shared only
with the corresponding signal on the other module.

Install **Adafruit BME680 Library**, **Adafruit MPU6050**, **Adafruit Unified
Sensor**, and **Adafruit BusIO** through Library Manager. A local copy of Arduino
SD 1.3.0 under `src/SoftwareSD` supports this wiring without an extra install.
Sensor output and `flight_m.csv` logging retain their existing format.

`BME680_SPI_Diagnostic` uses the same BME pin assignment for a hardware-SPI
communication test. `SPI_Loopback_Diagnostic` requires disconnected modules
and a temporary D11-D12 jumper; follow its separate instructions if needed.
