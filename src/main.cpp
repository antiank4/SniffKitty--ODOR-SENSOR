#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#define sensor_t adafruit_sensor_t
#include <Adafruit_AHTX0.h>
#undef sensor_t

#include "esp_camera.h"
#include "esp_http_server.h"

const char* ssid = "eduroam";
const char* password = "***REMOVED***";

const int MQ135_PIN = 1;
const int PIR_PIN = 2;

const int AHT_SDA = 14;
const int AHT_SCL = 21;

const int SAMPLE_COUNT = 5;
const int DELAY_BETWEEN_SAMPLES = 20;

int baseline = 0;

bool recording = false;
unsigned long lastTriggerTime = 0;
unsigned long recordingStartTime = 0;

const unsigned long PIR_COOLDOWN = 0;
const unsigned long PIR_IGNORE_AFTER_TRIGGER = 5000;

volatile bool pirInterruptTriggered = false;

Adafruit_AHTX0 aht;
bool ahtReady = false;
float baseTemp = 0;
float baseHum = 0;

float currentTempC = 0;
float currentHum = 0;
float currentDeltaTemp = 0;
float currentDeltaHum = 0;

int currentMQ135Raw = 0;
float currentVoltage = 0;
float currentChangePercent = 0;
String currentStatus = "Waiting";
bool currentPirState = false;

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11

#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

void IRAM_ATTR handlePirInterrupt() {
  pirInterruptTriggered = true;
}

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
  int count = 300;

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

void updateStatusFromMQ135() {
  if (currentChangePercent < 10) {
    currentStatus = "Normal";
  } else if (currentChangePercent < 30) {
    currentStatus = "Slight Change";
  } else if (currentChangePercent < 80) {
    currentStatus = "Medium Odor";
  } else {
    currentStatus = "Strong Odor!";
  }
}

void initCamera() {
  Serial.println("Camera init start...");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.pixel_format = PIXFORMAT_JPEG;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();

  if (s != NULL) {
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
  }

  Serial.println("Camera init success!");
}

static esp_err_t data_handler(httpd_req_t *req) {
  String json = "{";
  json += "\"time_ms\":" + String(millis()) + ",";
  json += "\"recording\":" + String(recording ? 1 : 0) + ",";
  json += "\"pir\":" + String(currentPirState ? 1 : 0) + ",";
  json += "\"mq135_raw\":" + String(currentMQ135Raw) + ",";
  json += "\"voltage\":" + String(currentVoltage, 3) + ",";
  json += "\"odor_change_percent\":" + String(currentChangePercent, 1) + ",";
  json += "\"status\":\"" + currentStatus + "\",";
  json += "\"temp_C\":" + String(currentTempC, 2) + ",";
  json += "\"hum_percent\":" + String(currentHum, 2) + ",";
  json += "\"delta_temp\":" + String(currentDeltaTemp, 2) + ",";
  json += "\"delta_hum\":" + String(currentDeltaHum, 2);
  json += "}";

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json.c_str(), json.length());
}

static esp_err_t index_handler(httpd_req_t *req) {
  const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Cat Monitor Dashboard</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #111827, #1f2937);
      color: white;
    }

    .page {
      padding: 24px;
    }

    .title {
      text-align: center;
      margin-bottom: 24px;
    }

    .title h1 {
      margin: 0;
      font-size: 32px;
    }

    .title p {
      color: #cbd5e1;
      margin-top: 8px;
    }

    .dashboard {
      display: grid;
      grid-template-columns: 2fr 1fr;
      gap: 24px;
      max-width: 1200px;
      margin: 0 auto;
    }

    .card {
      background: rgba(255, 255, 255, 0.08);
      border: 1px solid rgba(255, 255, 255, 0.12);
      border-radius: 20px;
      padding: 18px;
      box-shadow: 0 12px 32px rgba(0, 0, 0, 0.35);
      backdrop-filter: blur(8px);
    }

    .camera-box {
      background: #000;
      border-radius: 16px;
      overflow: auto;
      height: 75vh;
      display: flex;
      justify-content: center;
      align-items: flex-start;
    }

    #stream {
      transform-origin: center top;
      transform: scale(1);
      max-width: none;
    }

    .controls {
      margin-bottom: 14px;
      display: flex;
      gap: 10px;
      align-items: center;
      flex-wrap: wrap;
    }

    button {
      background: #38bdf8;
      color: #0f172a;
      font-weight: bold;
      border: none;
      border-radius: 10px;
      padding: 10px 14px;
      cursor: pointer;
    }

    button:hover {
      background: #7dd3fc;
    }

    .sensor-card {
      margin-bottom: 14px;
      background: rgba(15, 23, 42, 0.75);
      border-radius: 16px;
      padding: 16px;
      border: 1px solid rgba(148, 163, 184, 0.25);
    }

    .sensor-label {
      color: #94a3b8;
      font-size: 14px;
      margin-bottom: 6px;
    }

    .sensor-value {
      font-size: 26px;
      font-weight: bold;
    }

    .small {
      font-size: 14px;
      color: #cbd5e1;
      margin-top: 6px;
    }

    .status-pill {
      display: inline-block;
      padding: 8px 12px;
      border-radius: 999px;
      font-weight: bold;
    }

    .recording {
      background: #f97316;
      color: #431407;
    }

    .idle {
      background: #94a3b8;
      color: #0f172a;
    }

    @media (max-width: 900px) {
      .dashboard {
        grid-template-columns: 1fr;
      }

      .camera-box {
        height: 55vh;
      }
    }
  </style>
</head>

<body>
  <div class="page">
    <div class="title">
      <h1>Cat Health Monitor</h1>
      <p>Live camera with MQ135, AHT10 temperature/humidity, and PIR detection</p>
    </div>

    <div class="dashboard">
      <div class="card">
        <div class="controls">
          <button onclick="zoomIn()">Zoom In</button>
          <button onclick="zoomOut()">Zoom Out</button>
          <button onclick="resetZoom()">Reset</button>
          <span>Zoom: <b id="zoomText">100%</b></span>
        </div>

        <div class="camera-box">
          <img id="stream">
        </div>
      </div>

      <div class="card">
        <div class="sensor-card">
          <div class="sensor-label">System Status</div>
          <div>
            <span id="recordingStatus" class="status-pill idle">Idle</span>
          </div>
          <div class="small">PIR: <span id="pirStatus">0</span></div>
        </div>

        <div class="sensor-card">
          <div class="sensor-label">Temperature</div>
          <div class="sensor-value"><span id="temp">--</span> °C</div>
          <div class="small">Delta: <span id="deltaTemp">--</span> °C</div>
        </div>

        <div class="sensor-card">
          <div class="sensor-label">Humidity</div>
          <div class="sensor-value"><span id="hum">--</span> %</div>
          <div class="small">Delta: <span id="deltaHum">--</span> %</div>
        </div>

        <div class="sensor-card">
          <div class="sensor-label">MQ135 Raw</div>
          <div class="sensor-value"><span id="mq135">--</span></div>
          <div class="small">Voltage: <span id="voltage">--</span> V</div>
        </div>

        <div class="sensor-card">
          <div class="sensor-label">Odor Change</div>
          <div class="sensor-value"><span id="odor">--</span> %</div>
          <div class="small">Status: <b id="odorStatus">--</b></div>
        </div>

        <div class="sensor-card">
          <div class="sensor-label">Time</div>
          <div class="sensor-value"><span id="time">--</span> ms</div>
        </div>
      </div>
    </div>
  </div>

  <script>
    document.getElementById("stream").src = "http://" + location.hostname + ":8001/stream";

    let zoom = 1.0;

    function updateZoom() {
      const img = document.getElementById("stream");
      img.style.transform = "scale(" + zoom + ")";
      document.getElementById("zoomText").innerText = Math.round(zoom * 100) + "%";
    }

    function zoomIn() {
      zoom += 0.25;
      updateZoom();
    }

    function zoomOut() {
      if (zoom > 0.5) {
        zoom -= 0.25;
      }
      updateZoom();
    }

    function resetZoom() {
      zoom = 1.0;
      updateZoom();
    }

    async function updateData() {
      try {
        const response = await fetch("/data");
        const data = await response.json();

        document.getElementById("time").innerText = data.time_ms;
        document.getElementById("pirStatus").innerText = data.pir;
        document.getElementById("temp").innerText = Number(data.temp_C).toFixed(2);
        document.getElementById("hum").innerText = Number(data.hum_percent).toFixed(2);
        document.getElementById("deltaTemp").innerText = Number(data.delta_temp).toFixed(2);
        document.getElementById("deltaHum").innerText = Number(data.delta_hum).toFixed(2);
        document.getElementById("mq135").innerText = data.mq135_raw;
        document.getElementById("voltage").innerText = Number(data.voltage).toFixed(3);
        document.getElementById("odor").innerText = Number(data.odor_change_percent).toFixed(1);
        document.getElementById("odorStatus").innerText = data.status;

        const rec = document.getElementById("recordingStatus");

        if (data.recording === 1) {
          rec.innerText = "Recording";
          rec.className = "status-pill recording";
        } else {
          rec.innerText = "Idle";
          rec.className = "status-pill idle";
        }
      } catch (err) {
        console.log("Data update failed", err);
      }
    }

    setInterval(updateData, 1000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    char part_buf[64];

    size_t hlen = snprintf(
      part_buf,
      64,
      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      fb->len
    );

    res = httpd_resp_send_chunk(req, part_buf, hlen);

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    }

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "\r\n", 2);
    }

    esp_camera_fb_return(fb);

    if (res != ESP_OK) {
      break;
    }

    delay(50);
  }

  return res;
}

void startCameraServer() {
  httpd_config_t mainConfig = HTTPD_DEFAULT_CONFIG();
  mainConfig.server_port = 8000;
  mainConfig.max_uri_handlers = 8;
  mainConfig.ctrl_port = 32768;

  httpd_handle_t mainServer = NULL;

  if (httpd_start(&mainServer, &mainConfig) == ESP_OK) {
    httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL
    };

    httpd_uri_t data_uri = {
      .uri = "/data",
      .method = HTTP_GET,
      .handler = data_handler,
      .user_ctx = NULL
    };

    httpd_register_uri_handler(mainServer, &index_uri);
    httpd_register_uri_handler(mainServer, &data_uri);

    Serial.println("Dashboard server started on port 8000!");
  } else {
    Serial.println("Dashboard server failed to start");
  }

  httpd_config_t streamConfig = HTTPD_DEFAULT_CONFIG();
  streamConfig.server_port = 8001;
  streamConfig.max_uri_handlers = 4;
  streamConfig.ctrl_port = 32769;

  httpd_handle_t streamServer = NULL;

  if (httpd_start(&streamServer, &streamConfig) == ESP_OK) {
    httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL
    };

    httpd_register_uri_handler(streamServer, &stream_uri);

    Serial.println("Camera stream server started on port 8001!");
  } else {
    Serial.println("Camera stream server failed to start");
  }
}

void connectWiFi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. Open dashboard: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8000");
  Serial.print("Camera stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8001/stream");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  analogReadResolution(12);
  pinMode(PIR_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIR_PIN), handlePirInterrupt, RISING);

  Wire.begin(AHT_SDA, AHT_SCL);

  Serial.println("MQ135 + PIR interrupt + AHT10 + Web Dashboard start...");

  if (!aht.begin()) {
    Serial.println("AHT10 not found!");
    ahtReady = false;
  } else {
    Serial.println("AHT10 found!");
    ahtReady = true;
  }

  initCamera();

  connectWiFi();

  startCameraServer();

  calibrateBaseline();

  Serial.println("Waiting for PIR trigger...");
  Serial.println("CSV,time_ms,recording,pir,mq135_raw,voltage,odor_change_percent,status,temp_C,hum_percent,delta_temp,delta_hum");
}

void loop() {
  currentPirState = digitalRead(PIR_PIN);

  bool pirEvent = false;

  noInterrupts();
  if (pirInterruptTriggered) {
    pirInterruptTriggered = false;
    pirEvent = true;
  }
  interrupts();

  if (pirEvent) {
    if (millis() - lastTriggerTime > PIR_COOLDOWN) {
      recording = !recording;
      lastTriggerTime = millis();

      if (recording) {
        recordingStartTime = millis();

        Serial.println("PIR detected: START recording");
        Serial.print("Ignoring first ");
        Serial.print(PIR_IGNORE_AFTER_TRIGGER / 1000);
        Serial.println(" seconds of data after PIR trigger");

        if (ahtReady) {
          sensors_event_t humidity, temp;
          aht.getEvent(&humidity, &temp);

          baseTemp = temp.temperature;
          baseHum = humidity.relative_humidity;
        }
      } else {
        Serial.println("PIR detected: STOP recording");
      }
    }
  }

  if (ahtReady) {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    currentTempC = temp.temperature;
    currentHum = humidity.relative_humidity;

    if (recording) {
      currentDeltaTemp = currentTempC - baseTemp;
      currentDeltaHum = currentHum - baseHum;
    } else {
      currentDeltaTemp = 0;
      currentDeltaHum = 0;
    }
  }

  currentMQ135Raw = readMQ135Average();
  currentVoltage = currentMQ135Raw * (3.3 / 4095.0);
  currentChangePercent = ((float)(currentMQ135Raw - baseline) / baseline) * 100;
  updateStatusFromMQ135();

  Serial.print("Temp: ");
  Serial.print(currentTempC, 2);
  Serial.print(" C | Hum: ");
  Serial.print(currentHum, 2);
  Serial.print(" % | MQ135: ");
  Serial.print(currentMQ135Raw);
  Serial.print(" | PIR: ");
  Serial.print(currentPirState ? 1 : 0);
  Serial.print(" | Status: ");
  Serial.print(currentStatus);

  if (recording) {
    Serial.print(" | Delta Temp: ");
    Serial.print(currentDeltaTemp, 2);
    Serial.print(" C | Delta Hum: ");
    Serial.print(currentDeltaHum, 2);
    Serial.print(" %");
  }

  Serial.println();

  if (!recording) {
    delay(500);
    return;
  }

  if (millis() - recordingStartTime < PIR_IGNORE_AFTER_TRIGGER) {
    Serial.println("Ignoring unstable data after PIR trigger...");
    delay(1000);
    return;
  }

  Serial.print("CSV,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(recording ? 1 : 0);
  Serial.print(",");
  Serial.print(currentPirState ? 1 : 0);
  Serial.print(",");
  Serial.print(currentMQ135Raw);
  Serial.print(",");
  Serial.print(currentVoltage, 3);
  Serial.print(",");
  Serial.print(currentChangePercent, 1);
  Serial.print(",");
  Serial.print(currentStatus);
  Serial.print(",");
  Serial.print(currentTempC, 2);
  Serial.print(",");
  Serial.print(currentHum, 2);
  Serial.print(",");
  Serial.print(currentDeltaTemp, 2);
  Serial.print(",");
  Serial.println(currentDeltaHum, 2);

  delay(500);
}