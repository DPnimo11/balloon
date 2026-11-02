# Nano 33 BLE Sense Rev2 sensor sketch

Open `Nano33_BLE_Sense_Rev2.ino` in Arduino IDE as its own sketch.
The original `Balloon_Snesor_Code_Fall26.ino` remains unchanged.

1. In Boards Manager, install **Arduino Mbed OS Nano Boards** and select
   **Arduino Nano 33 BLE** and your board's USB port.
2. In Library Manager, install **Arduino_LPS22HB**, **Arduino_HS300x**,
   and **Arduino_BMI270_BMM150**, including their dependencies.
3. Upload and open Serial Monitor at **115200 baud**.

The sketch uses only the Rev2's onboard sensors. It prints a labeled debug
block once per second with temperature (C), pressure (hPa), relative humidity
(%), acceleration (g, including gravity), angular velocity (degrees/second),
and magnetic field (microteslas). All three IMU axes are included for each
measurement type. `Uptime` is milliseconds at the start of the sampling cycle;
the readings in a block are sequential, not simultaneous.

Each line includes its sensor name, units, and status: `OK`,
`INIT FAILED (reset to retry)`, or `NO VALID READING`.
IMU lines label the X, Y, and Z axes.
Missing readings are `nan`. Startup reports each sensor as ready or failed.
The sketch continues reading working sensors if another fails to
initialize; reset the board to retry initialization. USB startup waits at most
three seconds. Output goes to Serial only; this sketch does not save to SD.

Example output (illustrative values):

```text
--- Sample | Uptime: 4000 ms ---
Temperature (HS300x): 23.50 C | Status: OK
Pressure (LPS22HB): 1013.25 hPa | Status: OK
Relative humidity (HS300x): 45.20 % RH | Status: OK
Acceleration (BMI270) [g]: X=0.0100, Y=-0.0200, Z=1.0000 | Status: OK
Gyroscope (BMI270) [deg/s]: X=0.1000, Y=0.0000, Z=-0.1000 | Status: OK
Magnetic field (BMM150) [uT]: X=20.1000, Y=-5.3000, Z=40.2000 | Status: OK
```

This sketch specifically targets **Sense Rev2**. The original Sense revision
uses different IMU and temperature/humidity libraries. See Arduino's
[Rev2 hardware documentation](https://docs.arduino.cc/hardware/nano-33-ble-sense-rev2)
and the official [pressure](https://github.com/arduino-libraries/Arduino_LPS22HB),
[temperature/humidity](https://github.com/arduino-libraries/Arduino_HS300x),
and [IMU](https://github.com/arduino-libraries/Arduino_BMI270_BMM150) libraries.
