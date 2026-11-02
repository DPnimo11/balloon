/*
  Arduino Nano 33 BLE Sense Rev2 - onboard sensor reader.

  Install these libraries through Arduino IDE Library Manager:
    Arduino_LPS22HB        (pressure)
    Arduino_HS300x         (temperature and humidity)
    Arduino_BMI270_BMM150  (accelerometer, gyroscope, magnetometer)

  Board package: Arduino Mbed OS Nano Boards
  Board: Arduino Nano 33 BLE (also used for the Sense Rev2)
  Serial Monitor: 115200 baud

  Outputs labeled readings once per second. No external sensors or SD needed.
  Acceleration includes gravity. Gyroscope output is degrees/second.
  Missing/failed readings are nan, with a readable sensor status message.
  Open this sketch separately from the existing Uno sketch.
*/

#include <Arduino_LPS22HB.h>
#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <math.h>

const unsigned long SAMPLE_INTERVAL_MS = 1000;
const unsigned long IMU_TIMEOUT_MS = 200;
unsigned long lastSampleTime = 0;

bool pressureInitialized = false;
bool climateInitialized = false;
bool imuInitialized = false;

void printStatus(bool initialized, bool readingOK) {
  Serial.print(F(" | Status: "));
  if (!initialized) {
    Serial.println(F("INIT FAILED (reset to retry)"));
  } else if (!readingOK) {
    Serial.println(F("NO VALID READING"));
  } else {
    Serial.println(F("OK"));
  }
}

void printReading(const __FlashStringHelper *label, float value,
                  const __FlashStringHelper *unit,
                  bool initialized, bool readingOK) {
  Serial.print(label); Serial.print(F(": "));
  Serial.print(value, 2); Serial.print(' '); Serial.print(unit);
  printStatus(initialized, readingOK);
}

void printVector(const __FlashStringHelper *label,
                 const __FlashStringHelper *unit,
                 float x, float y, float z, bool readingOK) {
  Serial.print(label); Serial.print(F(" ["));
  Serial.print(unit); Serial.print(F("]: X="));
  Serial.print(x, 4); Serial.print(F(", Y="));
  Serial.print(y, 4); Serial.print(F(", Z="));
  Serial.print(z, 4);
  printStatus(imuInitialized, readingOK);
}

void setup() {
  Serial.begin(115200);
  // USB serial should not prevent startup when running on battery.
  const unsigned long serialStart = millis();
  while (!Serial && millis() - serialStart < 3000) {
    delay(10);
  }
  Serial.println(F("Nano 33 BLE Sense Rev2 - sensor debug"));

  pressureInitialized = BARO.begin();
  if (pressureInitialized) {
    // Continuous readings avoid the library's unbounded one-shot wait.
    BARO.setOutputRate(RATE_10_HZ);
    Serial.println(F("LPS22HB pressure sensor: READY"));
  } else {
    Serial.println(F("# LPS22HB pressure sensor initialization failed"));
  }

  climateInitialized = HS300x.begin();
  if (!climateInitialized) {
    Serial.println(F("# HS300x temperature/humidity sensor initialization failed"));
  } else {
    Serial.println(F("HS300x temperature/humidity sensor: READY"));
  }

  imuInitialized = IMU.begin();
  if (!imuInitialized) {
    Serial.println(F("# BMI270/BMM150 IMU initialization failed"));
  } else {
    Serial.println(F("BMI270/BMM150 IMU: READY"));
  }

  Serial.println(F("Readings every 1000 ms; acceleration includes gravity."));
  lastSampleTime = millis();
}

void loop() {
  const unsigned long now = millis();
  // Unsigned subtraction also handles millis() rollover.
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
  lastSampleTime = now;

  float temperatureC = NAN;
  float pressureHpa = NAN;
  float humidityPct = NAN;
  float ax = NAN, ay = NAN, az = NAN;
  float gx = NAN, gy = NAN, gz = NAN;
  float mx = NAN, my = NAN, mz = NAN;

  bool pressureOK = false;
  bool accelOK = false;
  bool gyroOK = false;
  bool magOK = false;

  if (pressureInitialized) {
    // MILLIBAR gives hPa; readPressure() defaults to kPa otherwise.
    pressureHpa = BARO.readPressure(MILLIBAR);
    pressureOK = isfinite(pressureHpa) && pressureHpa > 0.0f;
    if (!pressureOK) pressureHpa = NAN;
  }

  if (climateInitialized) {
    temperatureC = HS300x.readTemperature();
    humidityPct = HS300x.readHumidity();
  }

  if (imuInitialized) {
    const unsigned long imuStart = millis();
    // Check each channel independently so one missing channel cannot block
    // the other sensors. Start with nan each row to avoid reusing old data.
    while (millis() - imuStart < IMU_TIMEOUT_MS &&
           !(accelOK && gyroOK && magOK)) {
      if (!accelOK && IMU.accelerationAvailable()) {
        accelOK = IMU.readAcceleration(ax, ay, az) &&
                  isfinite(ax) && isfinite(ay) && isfinite(az);
      }
      if (!gyroOK && IMU.gyroscopeAvailable()) {
        gyroOK = IMU.readGyroscope(gx, gy, gz) &&
                 isfinite(gx) && isfinite(gy) && isfinite(gz);
      }
      if (!magOK && IMU.magneticFieldAvailable()) {
        magOK = IMU.readMagneticField(mx, my, mz) &&
                isfinite(mx) && isfinite(my) && isfinite(mz);
      }
      delay(1);
    }
  }

  if (!accelOK) ax = ay = az = NAN;
  if (!gyroOK) gx = gy = gz = NAN;
  if (!magOK) mx = my = mz = NAN;

  // Uptime marks the start of this sampling cycle; channels are read in order.
  Serial.println();
  Serial.print(F("--- Sample | Uptime: "));
  Serial.print(now); Serial.println(F(" ms ---"));
  printReading(F("Temperature (HS300x)"), temperatureC, F("C"),
               climateInitialized, isfinite(temperatureC));
  printReading(F("Pressure (LPS22HB)"), pressureHpa, F("hPa"),
               pressureInitialized, pressureOK);
  printReading(F("Relative humidity (HS300x)"), humidityPct, F("% RH"),
               climateInitialized, isfinite(humidityPct));
  printVector(F("Acceleration (BMI270)"), F("g"), ax, ay, az, accelOK);
  printVector(F("Gyroscope (BMI270)"), F("deg/s"), gx, gy, gz, gyroOK);
  printVector(F("Magnetic field (BMM150)"), F("uT"), mx, my, mz, magOK);
}
