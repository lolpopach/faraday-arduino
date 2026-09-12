// Coil voltage logger for the pendulum Faraday's law experiment.
// Samples the ADS1115 at 100 Hz and flashes a sync LED for 0.5 s at t = 0,
// which is what lines the log up with the video. See README.md.
//
// Copyright (c) 2026 Ui Chan Kim, Ye Geon Kim, Chan Hee Yang, Yongseok Jeong
// MIT licence -- see LICENSE.

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// =========================
// Pin settings
// =========================
const int SYNC_LED_PIN = 7;

// =========================
// Sampling settings
// =========================
const unsigned long SAMPLE_INTERVAL_US = 10000;   // 100 Hz
const unsigned long LED_DURATION_US = 500000;     // 0.5 s

// =========================
// Runtime variables
// =========================
unsigned long startTime_us = 0;
unsigned long lastSampleTime_us = 0;
unsigned long ledOnTime_us = 0;

bool measuring = false;
bool ledOn = false;
bool headerPrinted = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(SYNC_LED_PIN, OUTPUT);
  digitalWrite(SYNC_LED_PIN, LOW);

  Wire.begin();

  if (!ads.begin(0x48)) {
    Serial.println("ERROR,ADS1115_NOT_FOUND");

    // ADS1115 not answering: blink the LED fast, forever
    while (1) {
      digitalWrite(SYNC_LED_PIN, HIGH);
      delay(100);
      digitalWrite(SYNC_LED_PIN, LOW);
      delay(100);
    }
  }

  // +-1.024 V full scale, about 0.03125 mV per bit
  ads.setGain(GAIN_FOUR);

  // Internal conversion rate of the ADS1115
  ads.setDataRate(RATE_ADS1115_860SPS);

  Serial.println("READY");
  Serial.println("TYPE start THEN PRESS ENTER");
  Serial.println("TYPE stop THEN PRESS ENTER");
  Serial.println("CSV WILL START AFTER start COMMAND");
}

void startMeasurement() {
  measuring = true;
  ledOn = true;
  headerPrinted = false;

  startTime_us = micros();
  lastSampleTime_us = startTime_us;
  ledOnTime_us = startTime_us;

  // Sync marker on: this instant is t = 0
  digitalWrite(SYNC_LED_PIN, HIGH);

  // CSV header
  Serial.println("time_s,voltage_mV");
  headerPrinted = true;
}

void stopMeasurement() {
  measuring = false;
  ledOn = false;

  digitalWrite(SYNC_LED_PIN, LOW);

  Serial.println("STOPPED");
  Serial.println("TYPE start THEN PRESS ENTER TO START AGAIN");
}

void readCommand() {
  if (Serial.available() <= 0) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) {
    return;
  }

  if (cmd == "s" || cmd == "S" || cmd == "start" || cmd == "START") {
    if (!measuring) {
      startMeasurement();
    }
    return;
  }

  if (cmd == "x" || cmd == "X" || cmd == "stop" || cmd == "STOP") {
    if (measuring) {
      stopMeasurement();
    }
    return;
  }

  Serial.print("UNKNOWN_COMMAND,");
  Serial.println(cmd);
}

void loop() {
  // =========================
  // Check serial command
  // =========================
  readCommand();

  // Print nothing until a measurement has been started
  if (!measuring) {
    return;
  }

  unsigned long now_us = micros();

  // =========================
  // LED OFF after sync duration
  // =========================
  if (ledOn && now_us - ledOnTime_us >= LED_DURATION_US) {
    digitalWrite(SYNC_LED_PIN, LOW);
    ledOn = false;
  }

  // =========================
  // ADC sampling
  // =========================
  if (now_us - lastSampleTime_us >= SAMPLE_INTERVAL_US) {
    lastSampleTime_us += SAMPLE_INTERVAL_US;

    int16_t adcValue = ads.readADC_Differential_0_1();

    float voltage_V = ads.computeVolts(adcValue);
    float voltage_mV = voltage_V * 1000.0;

    float time_s = (now_us - startTime_us) / 1000000.0;

    Serial.print(time_s, 6);
    Serial.print(",");
    Serial.println(voltage_mV, 6);
  }
}
