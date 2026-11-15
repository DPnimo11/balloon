# Nano 33 BLE Sense Rev2 balloon logger

Open `nano_33.ino` from this folder. This is a separate Arduino sketch from the
Uno program in `Balloon_Snesor_Code_Fall26`.

The sketch logs once per second to a microSD card and USB Serial Monitor at
**115200 baud**. It reads the external analog UV sensor plus the onboard
HS3003 temperature/humidity, LPS22HB pressure, BMI270 accelerometer/gyroscope,
and BMM150 magnetometer. It does not stream Bluetooth, microphone audio, or
the onboard color/proximity/gesture sensor.

## Wiring

| Device connection | Nano pin |
| --- | --- |
| Adafruit GUVA-S12SD UV OUT | A1 |
| UV V+ | 3V3 |
| UV GND | GND |
| SD CS | D10 |
| SD MOSI / DI / COPI | D11 |
| SD MISO / DO / CIPO | D12 |
| SD SCK / CLK | D13 |
| SD GND | GND |

Use an SD breakout compatible with **3.3 V logic**. Connect its power input
according to that module's specifications: a bare 3.3 V breakout uses 3V3,
while a module with a regulator may require a different supply at VCC/VIN.
Do not feed 5 V into any Nano signal pin. The exact SD module has not been
specified, so its power requirement still needs checking.

The onboard sensors require no external wiring. SD uses the board's standard
hardware SPI bus and the standard SD library, not the Uno sketch's custom
software SPI driver. Change `SD_CS_PIN` near the top if CS is wired elsewhere.

## Arduino setup

1. In Boards Manager, install **Arduino Mbed OS Nano Boards** by Arduino.
2. Select **Arduino Nano 33 BLE**, which also targets the Sense Rev2, and its port.
3. In Library Manager, install these Arduino libraries:
   - **Arduino_BMI270_BMM150**
   - **Arduino_HS300x**
   - **Arduino_LPS22HB**
   - **SD**
4. Insert a **FAT16/FAT32** formatted card (not exFAT), then upload.
5. Open Serial Monitor at **115200 baud** to see initialization status and CSV
   readings. Startup waits at most three seconds for USB, so battery operation
   works without Serial Monitor.

## Logged data

Each boot creates the first unused filename from `NANO000.CSV` to
`NANO999.CSV`. Earlier files are preserved; archive them when all names are used.
Each row is written and the file closed. If SD initialization or writing fails,
USB readings continue and SD initialization is retried every five seconds.
A successful recovery creates a new numbered file. Samples taken while SD is
unavailable are not buffered or recovered. Avoid removing the card while powered.

The CSV contains elapsed milliseconds since boot (not a calendar timestamp),
temperature in Celsius, relative humidity in percent, pressure in hPa,
UV raw ADC count/voltage/estimated index, acceleration in g (including gravity),
gyro in degrees/second, and magnetic field in microteslas. The `*_ok` fields
are 1 for successful readings and 0 for unavailable/failed readings. Missing
values appear as `nan`. Onboard sensors that fail initialization require a reset
to retry. Serial status messages start with `#`; SD files contain only CSV.

UV readings average 16 ADC conversions at 12-bit resolution (0-4095), using
the nominal 3.3 V rail as reference. `uv_raw` is the rounded average;
`uv_voltage` uses the unrounded average. Adjust `ADC_REFERENCE_V` if the
measured 3V3 rail differs. `uv_saturated=1` means at least one conversion reached
4095; the voltage/index is then clipped. A disconnected analog input cannot be
reliably detected by software.

The 240-370 nm description matches Adafruit's **GUVA-S12SD (product 1918)**.
`uv_index_est = uv_voltage / 0.1` is Adafruit's approximate conversion, not a
calibrated UV index. Powering this sensor from 3V3 keeps its output within the
Nano's supply range. A 400/440 nm light is outside its specified detection band.

References:

- [Arduino Nano 33 BLE Sense Rev2 manual](https://docs.arduino.cc/tutorials/nano-33-ble-sense-rev2/cheat-sheet/)
- [Arduino pin definitions](https://github.com/arduino/ArduinoCore-mbed/blob/main/variants/ARDUINO_NANO33BLE/pins_arduino.h)
- [Adafruit GUVA-S12SD specifications and UV index approximation](https://www.adafruit.com/product/1918)
