# 🏋️ Edge AI Physiotherapy Assistant (ESP32 + IMU + EMG)

## 📌 Overview  
This project is an **AI-powered physiotherapy assistant** built using an ESP32, an **IMU sensor**, and an **EMG sensor**.  
It classifies wrist rehabilitation exercises into three categories:  
- ✅ **Good Form**  
- ❌ **Bad Form**  
- 💤 **Rest**  

The aim is to help physiotherapy patients (and athletes) get **real-time feedback** on whether their exercises are being performed correctly.  

---

## 💡 Initial Idea vs. Prototype  
- **Initial vision:**  
  We planned a **full-body wearable system** with multiple IMUs and EMG modules across different joints and muscles. This would allow **comprehensive physiotherapy monitoring** — shoulder rehab, knee mobility, back posture correction, etc.  

- **Time constraints:**  
  Due to limited time and resources, we **downscaled to a wrist-only prototype**.  
  - Focus: **wrist curls** (a common physiotherapy and strength rehab exercise).  
  - Sensors: **1 × MPU6050 IMU** + **1 × EMG sensor** on forearm.  
  - Goal: Show that the concept works and can be expanded later into a **multi-sensor wearable suit**.  

This keeps the prototype simple while still proving the **feasibility of wearable AI-assisted physiotherapy**.  

---

## 🔧 Hardware Setup  
- **ESP32 Devkit C** (target MCU)  
- **MPU6050 IMU** (accelerometer + gyroscope via I²C)  
- **EMG sensor** (analog, connected to ESP32 ADC pin)  

Connections:  
- IMU → SDA/SCL pins of ESP32  
- EMG → GPIO 33 (ADC input)  

---

## 📊 Data Collection  
- **Labels:** `good_form`, `bad_form`, `rest`  
- **Duration per label:** ~4 minutes each (3 min training / 1 min testing)  
- **Sampling:**  
  - IMU (Accel + Gyro) @ ~43 Hz  
  - EMG (preprocessed to RMS, MAV, WL features)  

Data was collected using the **Edge Impulse Data Forwarder**.  

---

## 🧮 Feature Extraction (DSP)  
Processing was done in **Edge Impulse** using **Spectral Analysis**:  
- Scale axes: `1`  
- Decimation ratio: `1`  
- Filter: `none`  
- FFT type: `FFT`  
- FFT length: `16`  
- Log spectrum: ✅  
- Overlap FFT frames: ✅  
- Improve low-frequency resolution: ✅  

---

## 🤖 Model Architecture  
- **Learning block:** Classification (Neural Network)  
- **Suggested NN:**  
  - Input: spectral features (~100–150)  
  - Dense (32 units, ReLU)  
  - Dense (16 units, ReLU)  
  - Dense (3 units, Softmax) → `good_form`, `bad_form`, `rest`  
- Dropout: `0.2`  
- Optimizer: `Adam`  
- Epochs: ~40  
- Batch size: 16  

Target Accuracy: **85–95%** (demo level).  

---

## ⚡ Deployment  
- **Target device:** ESP32 DevkitC  
- **Memory budget:**  
  - RAM: 320 KB+  
  - Flash: 4 MB+  
- **Deployment steps:**  
  1. Train & test model in Edge Impulse.  
  2. Export via **Arduino Library**.  
  3. Upload inference code to ESP32.  
  4. Open Serial Monitor to see predictions.  
