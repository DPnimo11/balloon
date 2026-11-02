# Uno R3 SPI loopback test

1. Unplug all power. Disconnect every external wire from Uno pins D9-D13,
   including BME and SD connections. Both modules must be disconnected from
   those pins for this test.
2. Connect a single jumper directly between the Uno's D11 and D12 header pins.
3. Reconnect USB. Open `SPI_Loopback_Diagnostic.ino` as its own sketch, select
   **Arduino Uno**, upload, and open Serial Monitor at **115200**.
4. Send the `125000 Hz loopback` and `1000000 Hz loopback` results.
5. Unplug power and remove the D11-D12 jumper before reconnecting the modules
   or uploading the balloon sketch again.

The test sends six byte patterns twenty times at each speed and checks that
the received bytes match. It uses only the built-in SPI library.

- PASS at both speeds: Uno SPI data output and input work through this jumper
  under these conditions. This does not test the external SCK or CS pins,
  original sensor wires, module power, level shifting, or either module.
- FAIL: check that the correct sketch and board are selected, the jumper is
  firmly in D11/D12, and all other D9-D13 wires are disconnected. Failure alone
  does not prove that the Uno is damaged.

This does not change the main balloon sketch or the BME diagnostic.
