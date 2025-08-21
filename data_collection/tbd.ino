#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

const int emgPin = 33;        // Analog pin for EMG sensor
#define EMG_WINDOW 20         // Smaller window (20 samples @1kHz = 20ms)
#define EMG_SAMPLE_DELAY 1    // ~1kHz sampling

int emgBuffer[EMG_WINDOW];
int emgIndex = 0;
bool emgReady = false;

// ---- EMG Feature Extraction ----
float computeRMS(int *buf, int len) {
  long sumSq = 0;
  for (int i = 0; i < len; i++) sumSq += (long)buf[i] * buf[i];
  return sqrt((float)sumSq / len);
}

float computeMAV(int *buf, int len) {
  long sum = 0;
  for (int i = 0; i < len; i++) sum += abs(buf[i]);
  return (float)sum / len;
}

float computeWL(int *buf, int len) {
  long wl = 0;
  for (int i = 1; i < len; i++) wl += abs(buf[i] - buf[i - 1]);
  return (float)wl;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // more responsive
}

void loop() {
  // ---- Collect EMG samples ----
  emgBuffer[emgIndex++] = analogRead(emgPin);

  if (emgIndex >= EMG_WINDOW) {
    emgIndex = 0;
    emgReady = true;
  }

  // ---- If EMG window ready, grab IMU and push row ----
  if (emgReady) {
    emgReady = false;

    float rms = computeRMS(emgBuffer, EMG_WINDOW);
    float mav = computeMAV(emgBuffer, EMG_WINDOW);
    float wl  = computeWL(emgBuffer, EMG_WINDOW);

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // CSV row: ax,ay,az,gx,gy,gz,emg_rms,emg_mav,emg_wl
    Serial.print(a.acceleration.x); Serial.print(",");
    Serial.print(a.acceleration.y); Serial.print(",");
    Serial.print(a.acceleration.z); Serial.print(",");
    Serial.print(g.gyro.x); Serial.print(",");
    Serial.print(g.gyro.y); Serial.print(",");
    Serial.print(g.gyro.z); Serial.print(",");
    Serial.print(rms); Serial.print(",");
    Serial.print(mav); Serial.print(",");
    Serial.println(wl);
  }

  delay(EMG_SAMPLE_DELAY); // ~1kHz EMG sampling
}
