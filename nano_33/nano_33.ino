// Arduino Nano 33 BLE Sense Rev2 balloon logger.
// UV OUT -> A1. SD: CS -> D10, MOSI -> D11, MISO -> D12, SCK -> D13.
// Power the UV breakout from 3V3. All external signals must be 3.3 V compatible.
// See README.md for libraries, wiring, and the CSV format.

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_HS300x.h>
#include <Arduino_LPS22HB.h>
#include <math.h>
#include <stdio.h>

#if !defined(ARDUINO_ARDUINO_NANO33BLE)
#error "Select Arduino Nano 33 BLE using the Arduino Mbed OS Nano Boards package."
#endif

constexpr int UV_PIN = A1;
constexpr int SD_CS_PIN = 10;
constexpr int ADC_BITS = 12;
constexpr int ADC_MAX = (1 << ADC_BITS) - 1;
constexpr float ADC_REFERENCE_V = 3.3f; // Nominal 3V3 rail; adjust if measured.
constexpr int UV_SAMPLES = 16;
constexpr unsigned long LOG_INTERVAL_MS = 1000;
constexpr unsigned long SD_RETRY_MS = 5000;

const char CSV_HEADER[] =
  "time_ms,temp_c,humidity_pct,pressure_hpa,uv_raw,uv_voltage,uv_index_est,"
  "uv_saturated,accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,gyro_z_dps,"
  "mag_x_ut,mag_y_ut,mag_z_ut,ht_ok,pressure_ok,accel_ok,gyro_ok,mag_ok";

struct Sample {
  unsigned long timeMs = 0;
  float temperature = NAN, humidity = NAN, pressure = NAN;
  int uvRaw = 0;
  float uvVoltage = NAN, uvIndex = NAN;
  bool uvSaturated = false;
  float ax = NAN, ay = NAN, az = NAN;
  float gx = NAN, gy = NAN, gz = NAN;
  float mx = NAN, my = NAN, mz = NAN;
  bool htOK = false, pressureOK = false;
  bool accelOK = false, gyroOK = false, magOK = false;
};

Sample sample;
bool htReady = false, pressureReady = false, imuReady = false;
bool sdReady = false;
char logFilename[13] = "NANO000.CSV";
unsigned long lastLogMs = 0, lastSDRetryMs = 0;

void readSensors() {
  sample = Sample{}; // Missing readings start as NaN, never as old values.
  sample.timeMs = millis();

  uint32_t uvSum = 0;
  for (int i = 0; i < UV_SAMPLES; ++i) {
    const int raw = analogRead(UV_PIN);
    uvSum += raw;
    sample.uvSaturated |= (raw == ADC_MAX);
  }
  sample.uvRaw = (uvSum + UV_SAMPLES / 2) / UV_SAMPLES;
  sample.uvVoltage = (float(uvSum) / UV_SAMPLES) * ADC_REFERENCE_V / ADC_MAX;
  // Adafruit GUVA-S12SD approximation: 0.1 V per UV index unit.
  // This is an estimate, not a calibrated UV index measurement.
  sample.uvIndex = sample.uvVoltage / 0.1f;

  if (htReady) {
    sample.temperature = HS300x.readTemperature();
    sample.humidity = HS300x.readHumidity();
    sample.htOK = isfinite(sample.temperature) && isfinite(sample.humidity);
  }
  if (pressureReady) {
    sample.pressure = BARO.readPressure(MILLIBAR); // mbar == hPa.
    sample.pressureOK = isfinite(sample.pressure) && sample.pressure > 0;
    if (!sample.pressureOK) sample.pressure = NAN;
  }
  if (imuReady) {
    if (IMU.accelerationAvailable()) {
      sample.accelOK = IMU.readAcceleration(sample.ax, sample.ay, sample.az);
      if (!sample.accelOK) sample.ax = sample.ay = sample.az = NAN;
    }
    if (IMU.gyroscopeAvailable()) {
      sample.gyroOK = IMU.readGyroscope(sample.gx, sample.gy, sample.gz);
      if (!sample.gyroOK) sample.gx = sample.gy = sample.gz = NAN;
    }
    if (IMU.magneticFieldAvailable()) {
      sample.magOK = IMU.readMagneticField(sample.mx, sample.my, sample.mz);
      if (!sample.magOK) sample.mx = sample.my = sample.mz = NAN;
    }
  }
}

// Use exactly the same columns for USB serial output and the SD file.
void writeSample(Print& out) {
  out.print(sample.timeMs);             out.print(',');
  out.print(sample.temperature, 2);     out.print(',');
  out.print(sample.humidity, 2);        out.print(',');
  out.print(sample.pressure, 3);        out.print(',');
  out.print(sample.uvRaw);              out.print(',');
  out.print(sample.uvVoltage, 4);        out.print(',');
  out.print(sample.uvIndex, 2);          out.print(',');
  out.print(sample.uvSaturated);        out.print(',');
  out.print(sample.ax, 4);              out.print(',');
  out.print(sample.ay, 4);              out.print(',');
  out.print(sample.az, 4);              out.print(',');
  out.print(sample.gx, 3);              out.print(',');
  out.print(sample.gy, 3);              out.print(',');
  out.print(sample.gz, 3);              out.print(',');
  out.print(sample.mx, 3);              out.print(',');
  out.print(sample.my, 3);              out.print(',');
  out.print(sample.mz, 3);              out.print(',');
  out.print(sample.htOK);               out.print(',');
  out.print(sample.pressureOK);         out.print(',');
  out.print(sample.accelOK);            out.print(',');
  out.print(sample.gyroOK);             out.print(',');
  out.println(sample.magOK);
}

bool initSD() {
  SD.end();
  if (!SD.begin(SD_CS_PIN)) {
    if (Serial) Serial.println("# SD init failed; retrying in 5 seconds.");
    return false;
  }

  // A new file per boot/recovery preserves earlier flights (8.3 filenames).
  for (int index = 0; index < 1000; ++index) {
    snprintf(logFilename, sizeof(logFilename), "NANO%03d.CSV", index);
    if (SD.exists(logFilename)) continue;

    File file = SD.open(logFilename, FILE_WRITE);
    if (!file) {
      if (Serial) Serial.println("# Could not create SD log file.");
      return false;
    }
    file.clearWriteError();
    file.println(CSV_HEADER);
    const bool ok = (file.getWriteError() == 0);
    file.close();
    if (Serial) {
      if (ok) {
        Serial.print("# Logging to ");
        Serial.println(logFilename);
      } else {
        Serial.println("# SD header write failed.");
      }
    }
    return ok;
  }
  if (Serial) Serial.println("# NANO000.CSV through NANO999.CSV already exist.");
  return false;
}

bool logToSD() {
  File file = SD.open(logFilename, FILE_WRITE);
  if (!file) return false;
  file.clearWriteError();
  writeSample(file);
  const bool ok = (file.getWriteError() == 0);
  file.close(); // Commit each row instead of leaving a whole flight buffered.
  return ok;
}

void setup() {
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(UV_PIN, INPUT);
  analogReference(AR_VDD);
  analogReadResolution(ADC_BITS);

  Serial.begin(115200);
  const unsigned long serialStart = millis();
  // Battery-powered operation must start even without a USB host.
  while (!Serial && millis() - serialStart < 3000) delay(10);

  pressureReady = BARO.begin();
  if (pressureReady) {
    // Continuous mode avoids the library's unbounded one-shot wait on failure.
    BARO.setOutputRate(RATE_1_HZ);
  }
  htReady = HS300x.begin();
  imuReady = IMU.begin();
  if (Serial) {
    Serial.println("# Nano 33 BLE Sense Rev2: UV=A1, SD CS=D10.");
    Serial.print("# Temperature/humidity: ");
    Serial.println(htReady ? "ready" : "FAILED");
    Serial.print("# Pressure: ");
    Serial.println(pressureReady ? "ready" : "FAILED");
    Serial.print("# IMU: ");
    Serial.println(imuReady ? "ready" : "FAILED");
  }
  sdReady = initSD();
  lastSDRetryMs = lastLogMs = millis();
  if (Serial) Serial.println(CSV_HEADER);
}

void loop() {
  const unsigned long now = millis();
  if (!sdReady && now - lastSDRetryMs >= SD_RETRY_MS) {
    sdReady = initSD();
    lastSDRetryMs = millis();
  }
  if (millis() - lastLogMs < LOG_INTERVAL_MS) return;
  lastLogMs = millis();

  readSensors();
  if (Serial) writeSample(Serial);
  if (sdReady && !logToSD()) {
    sdReady = false;
    lastSDRetryMs = millis();
    if (Serial) Serial.println("# SD write failed; retrying in 5 seconds.");
  }
}
