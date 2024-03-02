#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "../config/config.h"
#include <PubSubClient.h>

// Pin definitions for board to camera internal connection
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

const char* ssid = SSID;
const char* password = PASSWORD;

// MQTT Broker

const char* mqtt_broker = MQTT_BROKER;
const char* topic = TOPIC;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const int mqtt_port = MQTT_PORT;

WiFiClient espClient;
PubSubClient client(espClient);

// For Aysnc
AsyncWebServer server(80);

// boolean takeNewPhoto = false;

void sendingImage();
void postingImage();
void wifi();
void callback(char *topic, byte *payload, unsigned int length);

void setup() {
  Serial.begin(9600);
  pinMode(33,OUTPUT);
  wifi();
  // delay(1000);
  
  Serial.println();



  // OV2640 Camera Module
  Serial.println("INIT Camera");
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
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if(err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x",err);
    return;
  }


  // MQTT Broker Connection
  client.setServer(mqtt_broker,mqtt_port);
  client.setCallback(callback);

  while(!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n",client_id.c_str());
    if(client.connect(client_id.c_str(),mqtt_username,mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");
    } else{
            Serial.print("Failed with state ");
            Serial.print(client.state());
            delay(2000);
    }
  }

  // Publish to mqtt topic
  client.subscribe(topic);

}

void loop() {
  // digitalWrite(33,HIGH);
  // delay(1000);
  // digitalWrite(33,LOW);
  client.loop();
} 

void wifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(33,LOW);
    delay(1000);
    digitalWrite(33,HIGH);
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(33,LOW);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  // Route for capturing image
  server.on("/capture",HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", "Taking Photo");
    sendingImage();
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

}

camera_fb_t* capture() {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  return fb;
}

// posting image to local google vision express server
void postingImage(camera_fb_t *fb){
  HTTPClient client;
  client.begin("http://well-illegally-ghoul.ngrok-free.app/imageUpdate"); // will be changing later
  client.addHeader("Content-Type","image/jpeg");
  int httpResponseCode = client.POST(fb->buf, fb->len);
  if(httpResponseCode == 200){
    Serial.println("Image posted");
  }
  else{
    Serial.println("Error in posting image");
    Serial.println(httpResponseCode);
  }

  client.end();
}

void sendingImage(){
  camera_fb_t *fb = capture();
  if(!fb || fb->format != PIXFORMAT_JPEG){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }
  else{
    postingImage(fb);
    esp_camera_fb_return(fb);
  }
}

// callback function for mqtt

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  String message;
  for (int i=0;i<length;i++) {
    message += (char) payload[i]; // converting byte to string
  }
  Serial.print(message);
  
  // using recieved message for communication
  if(message.equals("picture"))
      {
        sendingImage();
      }
  Serial.println();
  Serial.println("-----------------------");
}