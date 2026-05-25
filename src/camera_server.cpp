#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"

#include "config.h"
#include "sensors.h"
#include "dashboard_html.h"
#include "camera_server.h"

const char* ssid = "eduroam";
const char* password = "***REMOVED***";

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
  json += "\"post_recording\":" + String(postRecording ? 1 : 0) + ",";
  json += "\"pir\":" + String(currentPirState ? 1 : 0) + ",";
  json += "\"mq137_raw\":" + String(currentMQ137Raw) + ",";
  json += "\"voltage\":" + String(currentVoltage, 3) + ",";
  json += "\"ammonia_change_percent\":" + String(currentChangePercent, 1) + ",";
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
  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  return httpd_resp_send(req, DASHBOARD_HTML, HTTPD_RESP_USE_STRLEN);
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

    delay(5);
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
