#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// UART
#define RXD2 16
#define TXD2 17

// Colors
#define COLOR_GOOD TFT_GREEN
#define COLOR_WARN TFT_YELLOW
#define COLOR_BAD TFT_RED
#define COLOR_BG TFT_BLACK
#define COLOR_TEXT TFT_WHITE
#define COLOR_TITLE TFT_CYAN

// Данные для двух узлов
struct ClusterData {
    String ip = "-";
    String hostname = "-";
    float cpu_temp = 0;
    float cpu_percent = 0;
    float ram_percent = 0;
    float disk_percent = 0;
    String pg_status = "UNKNOWN";
    String replication = "UNKNOWN";
    int node = 0;
};

// 2 nodes in PostgreSQL cluster
ClusterData nodeData[2]; // 0=RPi5-1, 1=RPi5-2
int currentNode = 0;     // Currently displayed node

// Init btn for switching between nodes
#define BUTTON_PIN 0 // GPIO0 (built in esp32 BOOT btn)
bool lastButtonState = HIGH;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Cluster Monitor starting...");

    // Init UART
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

    // Init display
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(COLOR_BG);

    // Heading
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

    // Init switching on BOOT btn
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Indicate esp32 working mode (blink)
    pinMode(2, OUTPUT);
}

// Draw cool-looking progress bar for metrics
void drawProgressBar(int x, int y, int w, int h, uint8_t percent,
                     uint16_t color) {
    tft.drawRect(x, y, w, h, COLOR_TEXT);
    if (percent > 0) {
        tft.fillRect(x + 1, y + 1, (w - 2) * percent / 100, h - 2, color);
    }
}

uint16_t getStatusColor(float value, float warning, float critical) {
    if (value >= critical)
        return COLOR_BAD;
    if (value >= warning)
        return COLOR_WARN;
    return COLOR_GOOD;
}

void drawClusterStatus(int nodeIndex) {
    tft.fillScreen(COLOR_BG);

    // Hostname + IPv4
    tft.setTextColor(COLOR_TITLE);
    tft.setTextSize(2);
    tft.setCursor(5, 10);
    tft.println(nodeData[nodeIndex].hostname);

    tft.setTextSize(2);
    tft.setCursor(5, 30);
    tft.println("IP: " + nodeData[nodeIndex].ip);

    // PostgreSQL status
    tft.setTextSize(2);
    tft.setCursor(5, 60);
    tft.setTextColor(COLOR_TEXT);
    tft.print("PG: ");
    if (nodeData[nodeIndex].pg_status == "ONLINE") {
        tft.setTextColor(COLOR_GOOD);
    } else {
        tft.setTextColor(COLOR_BAD);
    }
    tft.println(nodeData[nodeIndex].pg_status);

    // Replication status
    tft.setCursor(5, 80);
    tft.setTextColor(COLOR_TEXT);
    tft.print("Repl:");
    if (nodeData[nodeIndex].replication == "streaming") {
        tft.setTextColor(COLOR_GOOD);
    } else {
        tft.setTextColor(COLOR_WARN);
    }
    tft.println(nodeData[nodeIndex].replication);

    // CPU
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(5, 100);
    tft.print("CPU: ");
    tft.print(nodeData[nodeIndex].cpu_percent, 1);
    tft.println("%");
    drawProgressBar(5, 120, 230, 10, nodeData[nodeIndex].cpu_percent,
                    getStatusColor(nodeData[nodeIndex].cpu_percent, 60, 80));

    // CPU temp
    tft.setCursor(5, 135);
    tft.print("Temp: ");
    tft.print(nodeData[nodeIndex].cpu_temp, 1);
    tft.println("C");
    drawProgressBar(5, 155, 230, 10, nodeData[nodeIndex].cpu_temp / 80 * 100,
                    getStatusColor(nodeData[nodeIndex].cpu_temp, 60, 70));

    // RAM
    tft.setCursor(5, 170);
    tft.print("RAM: ");
    tft.print(nodeData[nodeIndex].ram_percent, 1);
    tft.println("%");
    drawProgressBar(120, 170, 115, 10, nodeData[nodeIndex].ram_percent,
                    getStatusColor(nodeData[nodeIndex].ram_percent, 70, 90));

    // SSD
    tft.setCursor(5, 190);
    tft.print("SSD: ");
    tft.print(nodeData[nodeIndex].disk_percent, 1);
    tft.println("%");
    drawProgressBar(120, 190, 115, 10, nodeData[nodeIndex].disk_percent,
                    getStatusColor(nodeData[nodeIndex].disk_percent, 75, 90));

    // Uptime
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(5, 220);
    tft.print("Uptime: ");
    tft.print(millis() / 1000);
    tft.println("s");
}

void loop() {
    // Blink to indicate working mode
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 500) {
        digitalWrite(2, !digitalRead(2));
        lastBlink = millis();
    }

    // Read data coming from UART
    if (Serial2.available()) {
        String data = Serial2.readStringUntil('\n');
        Serial.println("Received data: " + data);

        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, data);

        if (!error) {
            // Current cluster only has 3 nodes: 0 and 1 as rpi5, and a laptop,
            // so filter them out
            int nodeIndex = doc["node"].as<int>() - 1;
            if (nodeIndex >= 0 && nodeIndex < 2) {
                nodeData[nodeIndex].ip = doc["ip"].as<String>();
                nodeData[nodeIndex].hostname = doc["hostname"].as<String>();
                nodeData[nodeIndex].cpu_temp = doc["cpu_temp"].as<float>();
                nodeData[nodeIndex].cpu_percent =
                    doc["cpu_percent"].as<float>();
                nodeData[nodeIndex].ram_percent =
                    doc["ram_percent"].as<float>();
                nodeData[nodeIndex].disk_percent =
                    doc["disk_percent"].as<float>();
                nodeData[nodeIndex].pg_status = doc["pg_status"].as<String>();
                nodeData[nodeIndex].replication =
                    doc["replication"].as<String>();
                nodeData[nodeIndex].node = nodeIndex + 1;

                // Update screen if needed
                if (currentNode == nodeIndex) {
                    drawClusterStatus(currentNode);
                }
            }
        }
    }

    // Swtich screens on BOOT btn
    bool buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW && lastButtonState == HIGH) {
        currentNode = 1 - currentNode; // 0->1 or 1->0
        drawClusterStatus(currentNode);
        delay(200); // debounce
    }
    lastButtonState = buttonState;

    // Automatically swap screens each 5 seconds
    static unsigned long lastSwitch = 0;
    if (millis() - lastSwitch > 5000) {
        currentNode = 1 - currentNode;
        drawClusterStatus(currentNode);
        lastSwitch = millis();
    }

    delay(10);
}
