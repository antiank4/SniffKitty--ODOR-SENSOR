#include <Arduino.h>

const int MQ135_PIN = 1;

const int SAMPLE_COUNT = 20;
const int DELAY_BETWEEN_SAMPLES = 50;

int baseline = 0;

int readMQ135Average() {
  long sum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(MQ135_PIN);
    delay(DELAY_BETWEEN_SAMPLES);
  }
  return sum / SAMPLE_COUNT;
}

void calibrateBaseline() {
  Serial.println("Calibrating... keep air clean");

  long sum = 0;
  int count = 50;

  for (int i = 0; i < count; i++) {
    int val = readMQ135Average();
    sum += val;
    Serial.print(".");
  }

  baseline = sum / count;

  Serial.println();
  Serial.print("Baseline = ");
  Serial.println(baseline);
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等电脑串口稳定

  analogReadResolution(12);

  Serial.println("MQ135 sensor start...");

  calibrateBaseline();

  // CSV 表头（只发一次）
  Serial.println("time_ms,raw,voltage,change_percent,status");
}

void loop() {
  int rawValue = readMQ135Average();

  float voltage = rawValue * (3.3 / 4095.0);

  float changePercent = ((float)(rawValue - baseline) / baseline) * 100;

  String status;

  if (changePercent < 10) {
    status = "Normal";
  } else if (changePercent < 30) {
    status = "Slight Change";
  } else if (changePercent < 80) {
    status = "Medium Odor";
  } else {
    status = "Strong Odor!";
  }

  // 给人看的 monitor 输出
  Serial.print("Raw: ");
  Serial.print(rawValue);

  Serial.print(" | V: ");
  Serial.print(voltage, 3);

  Serial.print(" | Δ%: ");
  Serial.print(changePercent, 1);
  Serial.print("%");

  Serial.print(" | ");
  Serial.println(status);

  // 给 Python 存 CSV 用的输出
  Serial.print("CSV,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(rawValue);
  Serial.print(",");
  Serial.print(voltage, 3);
  Serial.print(",");
  Serial.print(changePercent, 1);
  Serial.print(",");
  Serial.println(status);

  delay(1000);
}