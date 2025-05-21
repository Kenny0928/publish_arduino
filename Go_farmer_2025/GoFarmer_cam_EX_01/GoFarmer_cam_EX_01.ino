/* --註解區
    > Go_Farmer_Cam
    > 作者: Ken ,2024.04.18 (已由 AI 修改錯誤)
    > 功能: ESP32-CAM網絡攝像頭，供App Inventor讀取
    > 使用AP模式，創建自己的WiFi網絡
*/

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"

// WiFi網絡設置 (AP模式)
const char* ap_ssid = "ESP32-CAM-AP";      // ESP32創建的WiFi名稱
const char* ap_password = "12345678";      // ESP32創建的WiFi密碼

IPAddress ap_ip(192, 168, 4, 1);           // ESP32的IP地址
IPAddress ap_gateway(192, 168, 4, 1);      // 網關地址(與ESP32相同)
IPAddress ap_subnet(255, 255, 255, 0);     // 子網掩碼

// 添加閃光燈LED控制引腳定義
#define FLASH_LED_PIN 4                    // GPIO 4 控制板載LED

#define PART_BOUNDARY "123456789000000000000987654321"

// 這個結構體根據您使用的ESP32-CAM板子型號保持不變
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "相機模型未選擇"
#endif

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// 添加HTML頁面
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32-CAM 串流</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 20px;
      background-color: #f0f0f0;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: #fff;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    }
    img {
      max-width: 100%;
      height: auto;
      border-radius: 5px;
    }
    h1 {
      color: #333;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32-CAM 攝像頭串流</h1>
    <img src="/stream" alt="ESP32-CAM 串流">
  </div>
</body>
</html>
)rawliteral";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t web_httpd = NULL;  // 增加一個HTTP伺服器處理控制請求

static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char part_buf[128]; // 修正: char 指標陣列改為 char 陣列，並增加大小

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  // 允許跨域訪問
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
  // 額外添加Cache-Control頭，避免緩存問題
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");
  httpd_resp_set_hdr(req, "Expires", "0");

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      // Serial.println("相機捕獲失敗"); // 開發時可開啟，部署時可考慮關閉以提高性能
      res = ESP_FAIL;
    } else {
      if(fb->format != PIXFORMAT_JPEG){
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;
        if(!jpeg_converted){
          // Serial.println("JPEG壓縮失敗"); // 開發時可開啟
          res = ESP_FAIL;
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }
    if(res == ESP_OK){
      // 修正: 使用 sizeof(part_buf)
      size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){ // 如果是 frame2jpg 分配的內存
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
  }
  return res;
}

// 添加相機控制處理程序
static esp_err_t cmd_handler(httpd_req_t *req){
  char* buf;
  size_t buf_len;
  char variable[32] = {0,};
  char value[32] = {0,};
  
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req); // Internal Server Error
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
          httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
        // 成功解析 var 和 val
      } else {
        free(buf);
        httpd_resp_set_status(req, "400 Bad Request"); // 修正: 設定 400 狀態
        httpd_resp_send(req, "Missing var or val parameter", strlen("Missing var or val parameter")); // 發送錯誤訊息
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_set_status(req, "400 Bad Request"); // 修正: 設定 400 狀態
      httpd_resp_send(req, "Could not parse query string", strlen("Could not parse query string")); // 發送錯誤訊息
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_set_status(req, "400 Bad Request"); // 修正: 設定 400 狀態
    httpd_resp_send(req, "No query string provided", strlen("No query string provided")); // 發送錯誤訊息
    return ESP_FAIL;
  }
  
  // 設置響應頭
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_type(req, "text/plain");
  
  // 獲取相機傳感器
  sensor_t * s = esp_camera_sensor_get();
  if(!s){
    httpd_resp_send_500(req); // Sensor not available
    return ESP_FAIL;
  }
  
  int val = atoi(value);
  int res_sensor = 0; // 用於感測器設置函數的返回值
  bool operation_performed = true; // 標記操作是否被執行
  
  // 根據變數名稱設置相應參數
  if(!strcmp(variable, "framesize")) {
    if(val >= 0 && val <= 10) res_sensor = s->set_framesize(s, (framesize_t)val); else operation_performed = false;
  }
  else if(!strcmp(variable, "quality")) res_sensor = s->set_quality(s, val);
  else if(!strcmp(variable, "contrast")) res_sensor = s->set_contrast(s, val);
  else if(!strcmp(variable, "brightness")) res_sensor = s->set_brightness(s, val);
  else if(!strcmp(variable, "hmirror")) res_sensor = s->set_hmirror(s, val);
  else if(!strcmp(variable, "vflip")) res_sensor = s->set_vflip(s, val);
  else if(!strcmp(variable, "flash")) {
    pinMode(FLASH_LED_PIN, OUTPUT); // 確保引腳為輸出模式
    digitalWrite(FLASH_LED_PIN, val ? HIGH : LOW); // val 非0則亮，為0則滅
    res_sensor = 0; // 假設成功 (ESP_OK 通常是 0)
  }
  else {
    operation_performed = false; // 未知的變數
  }
  
  // 準備回應訊息
  char resp_buf[128]; // 增加緩衝區大小以容納更長的錯誤訊息
  if (!operation_performed) {
    snprintf(resp_buf, sizeof(resp_buf), "Unknown variable: %s. Valid vars: framesize, quality, contrast, brightness, hmirror, vflip, flash.\n", variable);
    httpd_resp_set_status(req, "400 Bad Request"); // 修正: 設定 400 狀態
    httpd_resp_send(req, resp_buf, strlen(resp_buf)); // 發送錯誤訊息
    return ESP_FAIL; // 返回錯誤，因為命令未被識別或參數無效
  } else if(res_sensor == 0){ // ESP_OK 通常是 0
    snprintf(resp_buf, sizeof(resp_buf), "OK %s = %d\n", variable, val);
    httpd_resp_send(req, resp_buf, strlen(resp_buf));
  } else {
    snprintf(resp_buf, sizeof(resp_buf), "Failed to set %s = %d, sensor error: %d\n", variable, val, res_sensor);
    // 雖然感測器操作失敗，但請求本身是有效的，所以返回 ESP_OK，但在內容中指明失敗
    // 或者，也可以考慮返回一個伺服器錯誤狀態，例如 500
    httpd_resp_set_status(req, "500 Internal Server Error"); // 可選：如果感測器設置失敗視為伺服器錯誤
    httpd_resp_send(req, resp_buf, strlen(resp_buf));
  }
  
  return ESP_OK;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;  // 使用標準HTTP端口80，提高兼容性
  // config.ctrl_port = 32768; // ESP-IDF v4.x 及更新版本中，ctrl_port 通常由 httpd_start 自動分配或不需要手動設置為不同於 server_port 的值
                            // 如果你的 ESP-IDF 版本較舊，可能需要明確設置。對於新版本，可以註釋掉或移除。

  // 啟動主要HTTP伺服器
  Serial.printf("Starting web server on port: %d\n", config.server_port);
  if (httpd_start(&web_httpd, &config) == ESP_OK) {
    // 註冊HTML頁面處理程序
    httpd_uri_t index_uri = {
      .uri       = "/",
      .method    = HTTP_GET,
      .handler   = index_handler,
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(web_httpd, &index_uri);
    
    // 註冊控制處理程序
    httpd_uri_t cmd_uri = {
      .uri       = "/control",
      .method    = HTTP_GET,
      .handler   = cmd_handler,
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(web_httpd, &cmd_uri);
  } else {
    Serial.println("Failed to start web server");
    return; // 如果主伺服器啟動失敗，則不繼續
  }
  
  // 啟動串流伺服器
  config.server_port = 81;  // 將串流放到不同端口
  // config.ctrl_port = 32769; // 同上，對於新版 ESP-IDF 可能不需要
  
  Serial.printf("Starting stream server on port: %d\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    // 註冊串流處理程序
    httpd_uri_t stream_uri = {
      .uri       = "/stream",
      .method    = HTTP_GET,
      .handler   = stream_handler,
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    
    Serial.println("Web server started on port: 80, Stream server started on port: 81");
    Serial.println("HTML Page: / | Stream: /stream | Control: /control?var=PARAMETER&val=VALUE");
  } else {
    Serial.println("Failed to start stream server");
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // 禁用瀕死檢測
  
  // 設置閃光燈引腳為輸出並關閉
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW); // 確保初始狀態為關閉
  
  Serial.begin(115200);
  Serial.setDebugOutput(false); // 可以設為 true 以便調試
  
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  // 降低解析度設置，以便手機更容易處理
  if(psramFound()){
    config.frame_size = FRAMESIZE_CIF;  // 352x288
    config.jpeg_quality = 15;
    config.fb_count = 2; // 使用 PSRAM 時可以用兩個幀緩衝區
  } else {
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 20; 
    config.fb_count = 1; // 沒有 PSRAM 時，一個幀緩衝區
  }
  
  // 相機初始化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("相機初始化失敗，錯誤 0x%x\n", err);
    return;
  }

  // 設置螢幕旋轉（可選）
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    // s->set_hmirror(s, 0);  // 水平鏡像 (0 = normal, 1 = mirror)
    // s->set_vflip(s, 1);    // 垂直翻轉 (0 = normal, 1 = flip)
  }

  // 設置AP模式 (創建自己的WiFi網絡)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("");
  Serial.println("AP模式已啟動");
  Serial.print("WiFi網絡名稱: ");
  Serial.println(ap_ssid);
  Serial.print("WiFi密碼: ");
  Serial.println(ap_password);
  Serial.print("攝像頭網頁伺服器IP位址: http://");
  Serial.print(ap_ip);
  Serial.println("/"); 
  Serial.print("攝像頭串流伺服器IP位址: http://");
  Serial.print(ap_ip);
  Serial.println(":81/stream");
  Serial.println("可通過以下方式控制參數: http://" + ap_ip.toString() + "/control?var=參數&val=值");
  Serial.println("參數列表: framesize(0-10), quality(0-63), brightness(-2至2), contrast(-2至2), hmirror(0/1), vflip(0/1), flash(0/1)");
  Serial.println("請用手機/電腦連接到上述WiFi網絡");
  
  // 啟動相機服務器
  startCameraServer();
}

void loop() {
  delay(10000); 
}

