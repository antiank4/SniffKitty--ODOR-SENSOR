#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "img_converters.h"

#include "config.h"
#include "sensors.h"
#include "dashboard_html.h"
#include "camera_server.h"
#include "status_led.h"

const char* ssid = "eduroam";
const char* password = "***REMOVED***";

bool cameraBaselineReady = false;
bool currentCameraPresent = false;
float currentCameraChangePercent = 0;
bool currentCameraBoxValid = false;
float currentCameraBoxXPercent = 0;
float currentCameraBoxYPercent = 0;
float currentCameraBoxWPercent = 0;
float currentCameraBoxHPercent = 0;

static uint8_t* cameraBaselineFrame = NULL;
static size_t cameraBaselineLength = 0;
static float cameraBaselineMean = 0;
static int cameraFrameWidth = 0;
static int cameraFrameHeight = 0;

static bool captureGrayscaleSnapshot(
  uint8_t** grayFrame,
  size_t* grayLength,
  float* grayMean,
  int* frameWidth,
  int* frameHeight,
  const char* label,
  bool showCaptureBlink
) {
  if (showCaptureBlink) {
    blinkStatusLedBlue(2, 40, 40);
  }

  camera_fb_t* fb = NULL;

  for (int attempt = 0; attempt < 5 && fb == NULL; attempt++) {
    fb = esp_camera_fb_get();

    if (fb == NULL) {
      delay(80);
    }
  }

  if (!fb) {
    Serial.print(label);
    Serial.println(": camera frame unavailable");
    return false;
  }

  size_t pixelCount = fb->width * fb->height;
  int width = fb->width;
  int height = fb->height;
  uint8_t* rgbFrame = (uint8_t*)malloc(pixelCount * 3);

  if (rgbFrame == NULL) {
    Serial.print(label);
    Serial.print(": RGB buffer allocation failed, bytes=");
    Serial.println(pixelCount * 3);
    esp_camera_fb_return(fb);
    return false;
  }

  bool converted = fmt2rgb888(fb->buf, fb->len, fb->format, rgbFrame);
  esp_camera_fb_return(fb);

  if (!converted) {
    Serial.print(label);
    Serial.println(": JPEG to RGB conversion failed");
    free(rgbFrame);
    return false;
  }

  uint8_t* gray = (uint8_t*)malloc(pixelCount);

  if (gray == NULL) {
    Serial.print(label);
    Serial.print(": gray buffer allocation failed, bytes=");
    Serial.println(pixelCount);
    free(rgbFrame);
    return false;
  }

  unsigned long sum = 0;

  for (size_t i = 0; i < pixelCount; i++) {
    size_t rgbIndex = i * 3;
    uint8_t value = (uint8_t)(
      ((uint16_t)rgbFrame[rgbIndex] + (uint16_t)rgbFrame[rgbIndex + 1] + (uint16_t)rgbFrame[rgbIndex + 2]) / 3
    );
    gray[i] = value;
    sum += value;
  }

  free(rgbFrame);

  *grayFrame = gray;
  *grayLength = pixelCount;
  *grayMean = pixelCount > 0 ? (float)sum / pixelCount : 0;
  *frameWidth = width;
  *frameHeight = height;

  return true;
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
  config.frame_size = psramFound() ? FRAMESIZE_QVGA : FRAMESIZE_QQVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

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

void captureCameraBaselineAfterDelay() {
  Serial.print("Camera baseline will be captured in ");
  Serial.print(CAMERA_BASELINE_DELAY_MS / 1000);
  Serial.println(" seconds. Keep the litter box empty.");

  unsigned long startTime = millis();
  while (millis() - startTime < CAMERA_BASELINE_DELAY_MS) {
    unsigned long remainingMs = CAMERA_BASELINE_DELAY_MS - (millis() - startTime);
    Serial.print("Baseline capture in ");
    Serial.print((remainingMs + 999) / 1000);
    Serial.println("s");
    setStatusLedBlue();
    delay(500);
    clearStatusLed();
    delay(500);
  }

  uint8_t* grayFrame = NULL;
  size_t grayLength = 0;
  float grayMean = 0;
  int grayWidth = 0;
  int grayHeight = 0;

  if (!captureGrayscaleSnapshot(&grayFrame, &grayLength, &grayMean, &grayWidth, &grayHeight, "Camera baseline", false)) {
    Serial.println("Camera baseline capture failed.");
    cameraBaselineReady = false;
    return;
  }

  if (cameraBaselineFrame != NULL) {
    free(cameraBaselineFrame);
    cameraBaselineFrame = NULL;
  }

  cameraBaselineFrame = grayFrame;
  cameraBaselineLength = grayLength;
  cameraBaselineMean = grayMean;
  cameraFrameWidth = grayWidth;
  cameraFrameHeight = grayHeight;

  currentCameraChangePercent = 0;
  currentCameraPresent = false;
  currentCameraBoxValid = false;
  cameraBaselineReady = true;

  Serial.print("Camera baseline captured. Bytes: ");
  Serial.println(cameraBaselineLength);
}

bool updateCameraPresence() {
  static unsigned long lastCheckTime = 0;
  static int presentSamples = 0;
  static int emptySamples = CAMERA_EMPTY_SAMPLE_MIN;

  if (!cameraBaselineReady || cameraBaselineFrame == NULL) {
    currentCameraPresent = false;
    currentCameraChangePercent = 0;
    currentCameraBoxValid = false;
    return currentCameraPresent;
  }

  if (millis() - lastCheckTime < CAMERA_CHECK_INTERVAL_MS) {
    return currentCameraPresent;
  }

  lastCheckTime = millis();

  uint8_t* grayFrame = NULL;
  size_t grayLength = 0;
  float grayMean = 0;
  int grayWidth = 0;
  int grayHeight = 0;

  if (!captureGrayscaleSnapshot(&grayFrame, &grayLength, &grayMean, &grayWidth, &grayHeight, "Camera presence", true)) {
    Serial.println("Camera presence capture failed.");
    return currentCameraPresent;
  }

  if (grayLength != cameraBaselineLength || grayWidth != cameraFrameWidth || grayHeight != cameraFrameHeight) {
    Serial.println("Camera frame size changed; presence check skipped.");
    free(grayFrame);
    return currentCameraPresent;
  }

  float brightnessShift = grayMean - cameraBaselineMean;
  int rawChangedSamples = 0;
  int adjustedChangedSamples = 0;
  int totalSamples = 0;
  int minX = cameraFrameWidth;
  int minY = cameraFrameHeight;
  int maxX = -1;
  int maxY = -1;

  for (size_t i = 0; i < cameraBaselineLength; i += CAMERA_COMPARE_SAMPLE_STEP) {
    int rawDiff = abs((int)grayFrame[i] - (int)cameraBaselineFrame[i]);
    if (rawDiff >= CAMERA_PIXEL_DIFF_THRESHOLD) {
      rawChangedSamples++;
    }

    float adjustedCurrent = (float)grayFrame[i] - brightnessShift;
    int adjustedDiff = abs((int)(adjustedCurrent - (float)cameraBaselineFrame[i]));
    if (adjustedDiff >= CAMERA_PIXEL_DIFF_THRESHOLD) {
      adjustedChangedSamples++;

      int x = i % cameraFrameWidth;
      int y = i / cameraFrameWidth;
      minX = min(minX, x);
      minY = min(minY, y);
      maxX = max(maxX, x);
      maxY = max(maxY, y);
    }

    totalSamples++;
  }

  free(grayFrame);

  if (totalSamples > 0) {
    currentCameraChangePercent = ((float)rawChangedSamples / totalSamples) * 100.0;
  } else {
    currentCameraChangePercent = 0;
  }

  float adjustedChangePercent = totalSamples > 0
    ? ((float)adjustedChangedSamples / totalSamples) * 100.0
    : 0;

  currentCameraBoxValid = adjustedChangePercent >= 1.0 && maxX >= minX && maxY >= minY;

  if (currentCameraBoxValid) {
    currentCameraBoxXPercent = ((float)minX / cameraFrameWidth) * 100.0;
    currentCameraBoxYPercent = ((float)minY / cameraFrameHeight) * 100.0;
    currentCameraBoxWPercent = ((float)(maxX - minX + 1) / cameraFrameWidth) * 100.0;
    currentCameraBoxHPercent = ((float)(maxY - minY + 1) / cameraFrameHeight) * 100.0;
  } else {
    currentCameraBoxXPercent = 0;
    currentCameraBoxYPercent = 0;
    currentCameraBoxWPercent = 0;
    currentCameraBoxHPercent = 0;
  }

  bool rawPresent = currentCameraPresent
    ? adjustedChangePercent > CAMERA_EMPTY_CHANGE_PERCENT
    : adjustedChangePercent >= CAMERA_PRESENT_CHANGE_PERCENT;

  if (rawPresent) {
    if (presentSamples < CAMERA_PRESENT_SAMPLE_MIN) {
      presentSamples++;
    }
    emptySamples = 0;
  } else {
    if (emptySamples < CAMERA_EMPTY_SAMPLE_MIN) {
      emptySamples++;
    }
    presentSamples = 0;
  }

  if (!currentCameraPresent && presentSamples >= CAMERA_PRESENT_SAMPLE_MIN) {
    currentCameraPresent = true;
    Serial.print("Camera state: PRESENT, change ");
    Serial.print(currentCameraChangePercent, 1);
    Serial.print("%, adjusted ");
    Serial.print(adjustedChangePercent, 1);
    Serial.println("%");
  } else if (currentCameraPresent && emptySamples >= CAMERA_EMPTY_SAMPLE_MIN) {
    currentCameraPresent = false;
    Serial.print("Camera state: EMPTY, change ");
    Serial.print(currentCameraChangePercent, 1);
    Serial.print("%, adjusted ");
    Serial.print(adjustedChangePercent, 1);
    Serial.println("%");
  }

  return currentCameraPresent;
}

static esp_err_t data_handler(httpd_req_t *req) {
  String json = "{";
  json += "\"time_ms\":" + String(millis()) + ",";
  json += "\"recording\":" + String(recording ? 1 : 0) + ",";
  json += "\"post_recording\":" + String(postRecording ? 1 : 0) + ",";
  json += "\"present\":" + String(currentPresenceState ? 1 : 0) + ",";
  json += "\"camera_baseline_ready\":" + String(cameraBaselineReady ? 1 : 0) + ",";
  json += "\"camera_change_percent\":" + String(currentCameraChangePercent, 1) + ",";
  json += "\"camera_box_valid\":" + String(currentCameraBoxValid ? 1 : 0) + ",";
  json += "\"camera_box_x\":" + String(currentCameraBoxXPercent, 1) + ",";
  json += "\"camera_box_y\":" + String(currentCameraBoxYPercent, 1) + ",";
  json += "\"camera_box_w\":" + String(currentCameraBoxWPercent, 1) + ",";
  json += "\"camera_box_h\":" + String(currentCameraBoxHPercent, 1) + ",";
  json += "\"mq137_raw\":" + String(currentMQ137Raw) + ",";
  json += "\"voltage\":" + String(currentVoltage, 3) + ",";
  json += "\"ammonia_change_percent\":" + String(currentChangePercent, 1) + ",";
  json += "\"status\":\"" + currentStatus + "\",";
  json += "\"mq135_raw\":" + String(currentMQ135Raw) + ",";
  json += "\"mq135_voltage\":" + String(currentMQ135Voltage, 3) + ",";
  json += "\"air_change_percent\":" + String(currentMQ135ChangePercent, 1) + ",";
  json += "\"air_status\":\"" + currentMQ135Status + "\",";
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
  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  return httpd_resp_send(req, DASHBOARD_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    if (!cameraBaselineReady) {
      delay(200);
      continue;
    }

    fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    uint8_t* jpgBuf = fb->buf;
    size_t jpgLen = fb->len;
    bool converted = false;

    if (fb->format != PIXFORMAT_JPEG) {
      if (!frame2jpg(fb, 80, &jpgBuf, &jpgLen)) {
        Serial.println("Camera JPEG conversion failed");
        esp_camera_fb_return(fb);
        return ESP_FAIL;
      }
      converted = true;
    }

    char part_buf[64];

    size_t hlen = snprintf(
      part_buf,
      64,
      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      jpgLen
    );

    res = httpd_resp_send_chunk(req, part_buf, hlen);

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)jpgBuf, jpgLen);
    }

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "\r\n", 2);
    }

    if (converted) {
      free(jpgBuf);
    }

    esp_camera_fb_return(fb);

    if (res != ESP_OK) {
      break;
    }

    delay(80);
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

  unsigned long startTime = millis();
  const unsigned long wifiTimeoutMs = 15000;

  while (WiFi.status() != WL_CONNECTED && millis() - startTime < wifiTimeoutMs) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi not connected. Continuing in offline SD logging mode.");
    return;
  }

  Serial.println();
  Serial.print("WiFi connected. Open dashboard: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8000");

  Serial.print("Camera stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8001/stream");
}
