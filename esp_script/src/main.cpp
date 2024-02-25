#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h> // read and write from flash memory
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>


// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define LED 33

const char *ssid = "XXXXXXXXXX";
const char *password = "XXXXXXXXXXXXXXXX";
const char *post_url = "http://192.168.0.108:8080/image"; // Location where images are POSTED
bool internet_connected = false;


camera_fb_t * capture();
String send_capture(camera_fb_t * fb);
camera_config_t config;
sensor_t * s;

#define FLASH 4

void setup()
{
  pinMode(FLASH, OUTPUT);
  pinMode(LED, OUTPUT);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }

  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  // camera_config_t config;
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;



  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  config.frame_size = FRAMESIZE_QVGA; //HQVGA;         /////////////////////// added : reduced iamge size
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // sensor_t * s = esp_camera_sensor_get();
  s = esp_camera_sensor_get();
  s->set_brightness(s, 2);     // -2 to 2
  s->set_contrast(s, 2);       // -2 to 2
  s->set_saturation(s, 2);     // -2 to 2

  camera_fb_t * fb = capture();

  if (!fb) return;

  String resp = send_capture(fb);
  Serial.println(resp);

  esp_camera_fb_return(fb);
  
  if (resp == "X") {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
  } 


  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
  delay(1000);
  esp_deep_sleep_start();
}


void loop() {

  // camera_fb_t * fb = capture();

  // if (!fb) return;

  // String resp = send_capture(fb);
  // Serial.println(resp);

  // esp_camera_fb_return(fb);
  
  // if (resp == "A") {
  //   return; // another capture to send
  // } else {
  //     esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
  //     delay(1000);
  //     esp_deep_sleep_start();
  // }





}

String send_capture(camera_fb_t * fb) {
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url

  http.begin(post_url); //HTTP

  Serial.print("[HTTP] POST...\n");
  // start connection and send HTTP header
  int httpCode = http.sendRequest("POST", fb->buf, fb->len); // we simply put the whole image in the post body.

  // httpCode will be negative on error
  String payload = "";
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println(payload);
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return payload;
}


camera_fb_t * capture(){
  camera_fb_t *fb = NULL;
  digitalWrite(FLASH, HIGH);
  delay(100);
  fb = esp_camera_fb_get();
  delay(400);
  digitalWrite(FLASH, LOW);
  if (!fb) Serial.println("Camera capture failed");
    // for debug
  Serial.println((fb->buf)[0]);
  Serial.println((fb->buf)[1]);
  Serial.println((fb->buf)[2]);
  return fb;
}