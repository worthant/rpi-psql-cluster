#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <Wire.h>

TFT_eSPI tft = TFT_eSPI();

// I2C
#define SDA_PIN 26
#define SCL_PIN 27
#define I2C_ADDR 0x08

// Буфер для данных
String receivedData = "";
bool newData = false;

// Colors
#define COLOR_GOOD TFT_GREEN
#define COLOR_WARN TFT_YELLOW
#define COLOR_BAD TFT_RED
#define COLOR_BG TFT_BLACK
#define COLOR_TEXT TFT_WHITE
#define COLOR_TITLE TFT_CYAN

// Данные кластера
struct ClusterData {
  String ip = "-";
  String hostname = "-";
  float cpu_temp = 0;
  float cpu_percent = 0;
  float ram_percent = 0;
  float disk_percent = 0;
  String pg_status = "UNKNOWN";
  String replication = "UNKNOWN";
} nodeData;

// Обработчик I2C
void receiveEvent(int howMany) {
  while (Wire.available()) {
    char c = Wire.read();
    if (c == '\n') {
      newData = true;
    } else {
      receivedData += c;
    }
  }
}

void requestEvent() {
  // Отправка подтверждения
  Wire.write('A');
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 starting...");
  
  // Инициализация дисплея
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(2);
  tft.setCursor(15, 20);
  tft.println("PostgreSQL");
  tft.setCursor(10, 45);
  tft.println("RPi5 Cluster");
  tft.setCursor(50, 70);
  tft.println("Monitor");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 120);
  tft.println("Waiting for data...");
  
  // Настройка I2C
  Wire.begin(I2C_ADDR, SDA_PIN, SCL_PIN);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  Serial.println("I2C initialized at address 0x08");
  
  // Индикатор работы
  pinMode(2, OUTPUT);
}

void drawProgressBar(int x, int y, int w, int h, uint8_t percent, uint16_t color) {
  tft.drawRect(x, y, w, h, COLOR_TEXT);
  if (percent > 0) {
    tft.fillRect(x+1, y+1, (w-2) * percent / 100, h-2, color);
  }
}

uint16_t getStatusColor(float value, float warning, float critical) {
  if (value >= critical) return COLOR_BAD;
  if (value >= warning) return COLOR_WARN;
  return COLOR_GOOD;
}

void drawClusterStatus() {
  tft.fillScreen(COLOR_BG);
  
  // Hostname + IPv4
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(2);
  tft.setCursor(5, 10);
  tft.println(nodeData.hostname);
  
  tft.setTextSize(2);
  tft.setCursor(5, 30);
  tft.println("IP: " + nodeData.ip);
  
  // PostgreSQL status
  tft.setTextSize(2);
  tft.setCursor(5, 60);
  tft.setTextColor(COLOR_TEXT);
  tft.print("PG: ");
  if (nodeData.pg_status == "ONLINE") {
    tft.setTextColor(COLOR_GOOD);
  } else {
    tft.setTextColor(COLOR_BAD);
  }
  tft.println(nodeData.pg_status);
  
  // Replication status
  tft.setCursor(5, 80);
  tft.setTextColor(COLOR_TEXT);
  tft.print("Repl: ");
  if (nodeData.replication == "streaming") {
    tft.setTextColor(COLOR_GOOD);
  } else {
    tft.setTextColor(COLOR_WARN);
  }
  tft.println(nodeData.replication);
  
  // CPU
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(5, 100);
  tft.print("CPU: ");
  tft.print(nodeData.cpu_percent, 1);
  tft.println("%");
  drawProgressBar(5, 120, 230, 10, nodeData.cpu_percent, getStatusColor(nodeData.cpu_percent, 60, 80));
  
  // CPU temp
  tft.setCursor(5, 135);
  tft.print("Temp: ");
  tft.print(nodeData.cpu_temp, 1);
  tft.println("C");
  drawProgressBar(5, 155, 230, 10, nodeData.cpu_temp / 80 * 100, getStatusColor(nodeData.cpu_temp, 60, 70));
  
  // RAM
  tft.setCursor(5, 170);
  tft.print("RAM: ");
  tft.print(nodeData.ram_percent, 1);
  tft.println("%");
  drawProgressBar(120, 170, 115, 10, nodeData.ram_percent, getStatusColor(nodeData.ram_percent, 70, 90));
  
  // SSD
  tft.setCursor(5, 190);
  tft.print("SSD: ");
  tft.print(nodeData.disk_percent, 1);
  tft.println("%");
  drawProgressBar(120, 190, 115, 10, nodeData.disk_percent, getStatusColor(nodeData.disk_percent, 75, 90));
  
  // Uptime
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(5, 220);
  tft.print("Uptime: ");
  tft.print(millis()/1000);
  tft.println("s");
}

void loop() {
  // Мигание для индикации
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    digitalWrite(2, !digitalRead(2));
    lastBlink = millis();
  }
  
  // Обработка новых данных
  if (newData) {
    Serial.print("Received data: ");
    Serial.println(receivedData);
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, receivedData);
    
    if (!error) {
      nodeData.ip = doc["ip"].as<String>();
      nodeData.hostname = doc["hostname"].as<String>();
      nodeData.cpu_temp = doc["cpu_temp"].as<float>();
      nodeData.cpu_percent = doc["cpu_percent"].as<float>();
      nodeData.ram_percent = doc["ram_percent"].as<float>();
      nodeData.disk_percent = doc["disk_percent"].as<float>();
      nodeData.pg_status = doc["pg_status"] | "UNKNOWN";
      nodeData.replication = doc["replication"] | "UNKNOWN";
      
      drawClusterStatus();
    }
    
    receivedData = "";
    newData = false;
  }
  
  delay(10);
}
