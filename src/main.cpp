#include <Arduino.h>

const int MQ135_PIN = 18;

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
  delay(1000);

  analogReadResolution(12);

  Serial.println("MQ135 sensor start...");

  calibrateBaseline();  
}

void loop() {
  int rawValue = readMQ135Average();

  float voltage = rawValue * (3.3 / 4095.0);

  float changePercent = ((float)(rawValue - baseline) / baseline) * 100;

  Serial.print("Raw: ");
  Serial.print(rawValue);

  Serial.print(" | V: ");
  Serial.print(voltage, 3);

  Serial.print(" | Δ%: ");
  Serial.print(changePercent, 1);
  Serial.print("%");

  if (changePercent < 10) {
    Serial.println(" | Normal");
  } else if (changePercent < 30) {
    Serial.println(" | Slight Change");
  } else if (changePercent < 80) {
    Serial.println(" | Medium Odor");
  } else {
    Serial.println(" | Strong Odor!");
  }

  delay(1000);
}