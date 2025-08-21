#include <ESP_SR.h>
#include <esp32-hal-sr.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <TBD_inferencing.h>   // Edge Impulse model

Adafruit_MPU6050 mpu;

const int emgPin = 33;
#define EMG_WINDOW 20
#define EMG_SAMPLE_DELAY 1

int emgBuffer[EMG_WINDOW];
int emgIndex = 0;
bool emgReady = false;

// EI expects 3000ms window -> ~150 rows * 9 features = 1350 values
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static int featureIndex = 0;

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
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("System ready: IMU + EMG + Model");
}

void loop() {
  emgBuffer[emgIndex++] = analogRead(emgPin);

  if (emgIndex >= EMG_WINDOW) {
    emgIndex = 0;
    emgReady = true;
  }

  if (emgReady) {
    emgReady = false;

    float rms = computeRMS(emgBuffer, EMG_WINDOW);
    float mav = computeMAV(emgBuffer, EMG_WINDOW);
    float wl  = computeWL(emgBuffer, EMG_WINDOW);

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Push 9 features into the buffer
    features[featureIndex++] = a.acceleration.x;
    features[featureIndex++] = a.acceleration.y;
    features[featureIndex++] = a.acceleration.z;
    features[featureIndex++] = g.gyro.x;
    features[featureIndex++] = g.gyro.y;
    features[featureIndex++] = g.gyro.z;
    features[featureIndex++] = rms;
    features[featureIndex++] = mav;
    features[featureIndex++] = wl;

    // When enough data (3000ms window) is collected → run inference
    if (featureIndex >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
      signal_t signal;
      numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

      ei_impulse_result_t result;
      EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

      if (res != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier (");
        Serial.print(res);
        Serial.println(")");
      }
      else {
        // Print results
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          Serial.print(result.classification[ix].label);
          Serial.print(": ");
          Serial.println(result.classification[ix].value, 6);
        }
      }

      // Reset buffer for next 3s window
      featureIndex = 0;
    }
  }

  delay(EMG_SAMPLE_DELAY);
}
