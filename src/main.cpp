#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"

const char* ssid = "eduroam";
const char* password = "***REMOVED***";

const int MQ135_PIN = 1;

const int SAMPLE_COUNT = 20;
const int DELAY_BETWEEN_SAMPLES = 50;

int baseline = 0;

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

  // 你可以改这里控制真实拍摄分辨率
  config.frame_size = FRAMESIZE_QVGA;   // 320x240
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

static esp_err_t index_handler(httpd_req_t *req) {
  const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Camera</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial;
      text-align: center;
      background: #111;
      color: white;
      margin: 0;
      padding: 20px;
    }

    h2 {
      margin-bottom: 10px;
    }

    button {
      font-size: 16px;
      padding: 10px 15px;
      margin: 5px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
    }

    #viewer {
      margin: 20px auto 0 auto;
      overflow: auto;
      width: 100%;
      height: 75vh;
      border: 2px solid #444;
      background: #000;
      display: flex;
      justify-content: center;
      align-items: flex-start;
    }

    #stream {
      transform-origin: center top;
      transform: scale(1);
    }
  </style>
</head>

<body>
  <h2>ESP32 Live Camera</h2>

  <button onclick="zoomIn()">Zoom In</button>
  <button onclick="zoomOut()">Zoom Out</button>
  <button onclick="resetZoom()">Reset</button>

  <p>Zoom: <span id="zoomText">100%</span></p>

  <div id="viewer">
    <img id="stream" src="/stream">
  </div>

  <script>
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
  </script>
</body>
</html>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");

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
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 8000;

  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL
    };

    httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL
    };

    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &stream_uri);

    Serial.println("Camera server started!");
  } else {
    Serial.println("Camera server failed to start");
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
  Serial.print("WiFi connected. Open this URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8000");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  analogReadResolution(12);

  Serial.println("MQ135 sensor start...");

  initCamera();

  connectWiFi();

  startCameraServer();

  calibrateBaseline();

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

  Serial.print("Raw: ");
  Serial.print(rawValue);

  Serial.print(" | V: ");
  Serial.print(voltage, 3);

  Serial.print(" | Δ%: ");
  Serial.print(changePercent, 1);
  Serial.print("%");

  Serial.print(" | ");
  Serial.println(status);

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