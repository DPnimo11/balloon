#include <Wire.h>
#include <SPI.h>
#include "src/SensorPins.h"
#include "src/SoftwareSD/SD.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_MPU6050.h>

// -------------------- Pins --------------------
const int BME_CS_PIN = BalloonPins::BME_CS;
const int SD_CS_PIN  = BalloonPins::SD_CS;
const int UV_PIN     = A0;

// Shared SOFTWARE SPI pins matching the existing Uno R3 wiring:
// MOSI / module SDI = D11
// MISO = D12
// SCK  = D13
// BME680 CS = D9, SD CS = D10. Configuration: src/SensorPins.h.
// MPU6050 I2C on Uno R3: SDA = A4, SCL = A5.

// -------------------- Timing --------------------
const unsigned long LOG_INTERVAL_MS = 1000;
unsigned long lastLogTime = 0;

// -------------------- Sensors --------------------
Adafruit_BME680 bme(BME_CS_PIN, BalloonPins::BUS_MOSI,
                   BalloonPins::BUS_MISO, BalloonPins::BUS_SCK);
Adafruit_MPU6050 mpu;
uint8_t mpuAddress = 0x68; // AD0 low; init also tries 0x69 (AD0 high).
bool mpuInitialized = false;
// New filename keeps MPU columns separate from existing BNO flight.csv data.
const char LOG_FILENAME[] = "flight_m.csv";
File dataFile;

// -------------------- Status --------------------
bool bmeOK = false;
bool bmeInitialized = false;
bool mpuOK = false;
bool sdOK  = false;

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  digitalWrite(BME_CS_PIN, HIGH);
  pinMode(BME_CS_PIN, OUTPUT);

  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);

  digitalWrite(BalloonPins::BUS_SCK, LOW);
  pinMode(BalloonPins::BUS_SCK, OUTPUT);
  digitalWrite(BalloonPins::BUS_MOSI, HIGH);
  pinMode(BalloonPins::BUS_MOSI, OUTPUT);
  pinMode(BalloonPins::BUS_MISO, INPUT);

  Serial.print(F("Software SPI: SDI=D")); Serial.print(BalloonPins::BUS_MOSI);
  Serial.print(F(", SDO=D")); Serial.print(BalloonPins::BUS_MISO);
  Serial.print(F(", SCK=D")); Serial.print(BalloonPins::BUS_SCK);
  Serial.print(F("; BME CS=D")); Serial.print(BME_CS_PIN);
  Serial.print(F(", SD CS=D")); Serial.println(SD_CS_PIN);

  Wire.begin();

  initBME680();
  initMPU6050();
  initSD();
}

// -------------------- Loop --------------------
void loop() {
  if (millis() - lastLogTime < LOG_INTERVAL_MS) return;
  lastLogTime = millis();

  unsigned long t = millis();

  float tempC = NAN;
  float press_hPa = NAN;
  float hum_pct = NAN;
  float gas_ohms = NAN;

  int uvRaw = -1;
  float uvVoltage = NAN;
  int uvIndex = -1;

  float accelX = NAN;
  float accelY = NAN;
  float accelZ = NAN;
  float gyroX = NAN;
  float gyroY = NAN;
  float gyroZ = NAN;
  float mpuTempC = NAN;

  // ---------- BME680 ----------
  digitalWrite(SD_CS_PIN, HIGH);
  if (bmeInitialized && bme.performReading()) {
    bmeOK = true;
    tempC = bme.temperature;
    press_hPa = bme.pressure / 100.0;   // Pa -> hPa
    hum_pct = bme.humidity;
    gas_ohms = bme.gas_resistance;
  } else {
    bmeOK = false;
  }

  // ---------- UV ----------
  uvRaw = analogRead(UV_PIN);
  uvVoltage = uvRaw * (5.0 / 1023.0);
  uvIndex = (int)(uvVoltage / 0.1);   // Adafruit rough conversion

  // ---------- MPU6050 ----------
  mpuOK = false;
  if (mpuInitialized) {
    sensors_event_t accel, gyro, mpuTemp;
    // Check presence: the library's getEvent() does not report I2C failures.
    Wire.beginTransmission(mpuAddress);
    if (Wire.endTransmission() == 0 && mpu.getEvent(&accel, &gyro, &mpuTemp)) {
      mpuOK = true;
      // Acceleration includes gravity (m/s^2); angular velocity is rad/s.
      accelX = accel.acceleration.x;
      accelY = accel.acceleration.y;
      accelZ = accel.acceleration.z;
      gyroX = gyro.gyro.x;
      gyroY = gyro.gyro.y;
      gyroZ = gyro.gyro.z;
      mpuTempC = mpuTemp.temperature;
    }
  }

  printDebug(
    t,
    tempC, press_hPa, hum_pct, gas_ohms,
    uvRaw, uvVoltage, uvIndex,
    accelX, accelY, accelZ,
    gyroX, gyroY, gyroZ, mpuTempC
  );

  logData(
    t,
    tempC, press_hPa, hum_pct, gas_ohms,
    uvRaw, uvVoltage, uvIndex,
    accelX, accelY, accelZ,
    gyroX, gyroY, gyroZ, mpuTempC
  );
}

// -------------------- Init --------------------
void initBME680() {
  digitalWrite(SD_CS_PIN, HIGH); // disable SD
  if (!bme.begin()) {
    Serial.println(F("BME680 init failed"));
    bmeOK = false;
    return;
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  bmeOK = true;
  bmeInitialized = true;
  Serial.println(F("BME680 ready"));
}

void initMPU6050() {
  Serial.println(F("Initializing MPU6050 at 0x68 or 0x69..."));
  mpuAddress = 0x68;
  mpuInitialized = mpu.begin(mpuAddress, &Wire);
  if (!mpuInitialized) {
    mpuAddress = 0x69;
    mpuInitialized = mpu.begin(mpuAddress, &Wire);
  }
  if (!mpuInitialized) {
    Serial.println(F("MPU6050 init failed"));
    mpuOK = false;
    // Only scan after failure so the startup output identifies address/wiring issues.
    Serial.println(F("Scanning I2C bus (MPU6050 expected at 0x68 or 0x69)..."));
    bool foundDevice = false;
    for (uint8_t address = 1; address < 127; ++address) {
      Wire.beginTransmission(address);
      if (Wire.endTransmission() == 0) {
        foundDevice = true;
        Serial.print(F("I2C device at 0x"));
        if (address < 0x10) Serial.print('0');
        Serial.println(address, HEX);
      }
    }
    if (!foundDevice) {
      Serial.println(F("No I2C devices found. Check power, GND, SDA=A4, SCL=A5 on Uno R3."));
    }
    return;
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);
  mpuOK = true;
  Serial.print(F("MPU6050 ready at 0x"));
  Serial.println(mpuAddress, HEX);
}

void initSD() {
  digitalWrite(BME_CS_PIN, HIGH); // disable BME
  Serial.print(F("Initializing SD card..."));

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD init failed"));
    sdOK = false;
    return;
  }

  Serial.println(F("SD init done"));
  sdOK = false;

  dataFile = SD.open(LOG_FILENAME, FILE_WRITE);
  if (dataFile) {
    dataFile.clearWriteError();
    if (dataFile.size() == 0) {
      dataFile.println(F("time_ms,temp_c,pressure_hpa,humidity_pct,gas_ohms,uv_raw,uv_voltage,uv_index,accel_x_mps2,accel_y_mps2,accel_z_mps2,gyro_x_rads,gyro_y_rads,gyro_z_rads,mpu_temp_c,bme_ok,mpu_ok,sd_ok"));
    }
    sdOK = (dataFile.getWriteError() == 0);
    dataFile.close();
    if (sdOK) {
      Serial.println(F("flight_m.csv ready"));
    } else {
      Serial.println(F("Error writing CSV header"));
    }
  } else {
    Serial.println(F("Error opening flight_m.csv"));
    sdOK = false;
  }
}

// -------------------- Logging --------------------
void logData(
  unsigned long t,
  float tempC, float press_hPa, float hum_pct, float gas_ohms,
  int uvRaw, float uvVoltage, int uvIndex,
  float accelX, float accelY, float accelZ,
  float gyroX, float gyroY, float gyroZ, float mpuTempC
) {
  if (!sdOK) return;

  digitalWrite(BME_CS_PIN, HIGH);
  dataFile = SD.open(LOG_FILENAME, FILE_WRITE);
  if (!dataFile) {
    Serial.println(F("Error opening flight_m.csv for append"));
    sdOK = false;
    return;
  }

  dataFile.clearWriteError();
  dataFile.print(t);         dataFile.print(',');
  dataFile.print(tempC);     dataFile.print(',');
  dataFile.print(press_hPa); dataFile.print(',');
  dataFile.print(hum_pct);   dataFile.print(',');
  dataFile.print(gas_ohms);  dataFile.print(',');
  dataFile.print(uvRaw);     dataFile.print(',');
  dataFile.print(uvVoltage); dataFile.print(',');
  dataFile.print(uvIndex);   dataFile.print(',');
  dataFile.print(accelX);    dataFile.print(',');
  dataFile.print(accelY);    dataFile.print(',');
  dataFile.print(accelZ);    dataFile.print(',');
  dataFile.print(gyroX);     dataFile.print(',');
  dataFile.print(gyroY);     dataFile.print(',');
  dataFile.print(gyroZ);     dataFile.print(',');
  dataFile.print(mpuTempC);  dataFile.print(',');
  dataFile.print(bmeOK);     dataFile.print(',');
  dataFile.print(mpuOK);     dataFile.print(',');
  dataFile.println(sdOK);

  bool writeFailed = (dataFile.getWriteError() != 0);
  dataFile.close();
  if (writeFailed) {
    Serial.println(F("Error writing to flight_m.csv"));
    sdOK = false;
  }
}

// -------------------- Debug --------------------
void printDebug(
  unsigned long t,
  float tempC, float press_hPa, float hum_pct, float gas_ohms,
  int uvRaw, float uvVoltage, int uvIndex,
  float accelX, float accelY, float accelZ,
  float gyroX, float gyroY, float gyroZ, float mpuTempC
) {
  Serial.print(F("t=")); Serial.print(t);

  Serial.print(F(" | BME: "));
  if (bmeOK) {
    Serial.print(F("T=")); Serial.print(tempC);
    Serial.print(F(" C, P=")); Serial.print(press_hPa);
    Serial.print(F(" hPa, H=")); Serial.print(hum_pct);
    Serial.print(F(" %, G=")); Serial.print(gas_ohms);
    Serial.print(F(" ohms"));
  } else {
    Serial.print(F("FAIL"));
  }

  Serial.print(F(" | UV: raw="));
  Serial.print(uvRaw);
  Serial.print(F(", V="));
  Serial.print(uvVoltage);
  Serial.print(F(", UVI~"));
  Serial.print(uvIndex);

  Serial.print(F(" | MPU6050: "));
  if (mpuOK) {
    Serial.print(F("accel[m/s^2] x=")); Serial.print(accelX);
    Serial.print(F(", y=")); Serial.print(accelY);
    Serial.print(F(", z=")); Serial.print(accelZ);
    Serial.print(F("; gyro[rad/s] x=")); Serial.print(gyroX);
    Serial.print(F(", y=")); Serial.print(gyroY);
    Serial.print(F(", z=")); Serial.print(gyroZ);
    Serial.print(F("; T=")); Serial.print(mpuTempC);
    Serial.print(F(" C"));
  } else {
    Serial.print(F("FAIL"));
  }

  Serial.print(F(" | SD="));
  Serial.println(sdOK ? F("OK") : F("FAIL"));
}
