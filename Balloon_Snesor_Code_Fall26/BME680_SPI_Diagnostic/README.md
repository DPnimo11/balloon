# Uno R3 BME680 SPI diagnostic

Open `BME680_SPI_Diagnostic.ino` as its own sketch. Select **Arduino Uno**,
upload, and open Serial Monitor at **115200 baud**. No additional libraries
are needed; this uses the board package's built-in SPI library.

Keep the SD module fully disconnected, including signal wires. Leave the BME
on its existing power supply and ground, with CS on D9, SDI/MOSI on D11,
SDO/MISO on D12, and SCK on D13. Do not change the supply voltage without
checking the exact breakout's requirements. The MPU6050 can remain connected;
this sketch does not access I2C.

The test reads the BME68x chip ID directly at 125 kHz and 1 MHz, explicitly
selecting the correct SPI register page. It does not initialize sensors, run
the gas heater, measure the environment, or write to the SD card.

- Stable `0x61` at both speeds: basic chip-ID communication works. This does
  not verify calibration, measurements, or all connections under load. If the
  main sketch still fails, check its library versions and Uno memory usage.
- `0x61` at only one speed, or values that change: investigate intermittent
  connections, signal quality, and supply stability.
- Repeated `0x00` or `0xFF`: no valid chip ID was received. Check power, ground,
  CS, and the SPI signal paths. These values alone do not prove a dead sensor.
- Another stable ID: send the value and the module's markings for inspection.

The main balloon sketch is unchanged. Upload it again after troubleshooting.

Register addresses, page selection, and expected ID are based on Bosch's
[BME68x definitions](https://github.com/boschsensortec/BME68x_SensorAPI/blob/master/bme68x_defs.h)
and [SPI register access](https://github.com/boschsensortec/BME68x_SensorAPI/blob/master/bme68x.c).
