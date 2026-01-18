/*
  Project: Arduino PC Monitor (T-Display S3 Version)
  Hardware: LilyGO T-Display S3 (ESP32-S3)
  Library Requirements:
    1. TFT_eSPI (by Bodmer)
       *** 設定說明 (Setup Guide) ***
       請編輯 Arduino libraries/TFT_eSPI/User_Setup_Select.h：
       1. 註解掉 (Comment out): #include <User_Setup.h>
       2. 取消註解 (Uncomment): #include <User_Setups/Setup206_LilyGo_T_Display_S3.h>
    2. ArduinoJson (by Benoit Blanchon) - 建議使用 v7.x
*/

#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <SPI.h>

// --- Configuration ---
#define SCREEN_W 320
#define SCREEN_H 170

// Colors
#define C_BG TFT_BLACK
#define C_PANEL 0x18E3 // Dark Grey (RGB565)
#define C_TEXT TFT_WHITE
#define C_LABEL 0x9CD3 // Light Grey
#define C_ACCENT 0x067F // Cyan
#define C_WARN 0xF800   // Red
#define C_OK 0x07E0     // Green
#define C_GRID 0x2124   // Darker Grey
#define C_CPU 0x067F    // Cyan
#define C_RAM 0xF81F    // Magenta
#define C_DISK 0xFFE0   // Yellow
#define C_NET 0x07E0    // Green

// Objects
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite largeSprite = TFT_eSprite(&tft);
TFT_eSprite smallSprite = TFT_eSprite(&tft);

// Global Variables for Graph
#define GRAPH_W 71      // 75px Panel - 4px Padding
#define GRAPH_H_L 50    // Large Graph Height (CPU/RAM)
#define GRAPH_H_S 35    // Small Graph Height (Disk/Net)

float cpuHistory[GRAPH_W];
float ramHistory[GRAPH_W];
float diskReadHistory[GRAPH_W];
float diskWriteHistory[GRAPH_W];
float netDlHistory[GRAPH_W];
float netUlHistory[GRAPH_W];

#define HEADER_STRING_Y 18

unsigned long lastDataTime = 0;


void setup() {
  Serial.begin(115200);
  
  // Initialize Screen
  tft.init();
  tft.setRotation(1); // Landscape (USB right) or 3 (USB left)
  tft.fillScreen(C_BG);
  tft.setTextDatum(TL_DATUM);

  // Initialize Sprite for flicker-free graph
  largeSprite.createSprite(GRAPH_W, GRAPH_H_L);
  smallSprite.createSprite(GRAPH_W, GRAPH_H_S);
  
  // Initialize History Buffer
  for(int i=0; i<GRAPH_W; i++) {
    cpuHistory[i] = 0; ramHistory[i] = 0;
    diskReadHistory[i] = 0; diskWriteHistory[i] = 0;
    netDlHistory[i] = 0; netUlHistory[i] = 0;
  }

  // Draw Static Interface Layout
  drawStaticLayout();
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      parseAndDisplay(input, true);
      lastDataTime = millis();
    }
  } else {
    // Simulation Mode: If no data for 2 seconds, generate fake data
    if (millis() - lastDataTime > 1000) {
      simulateData();
      lastDataTime = millis();
    }
  }
}

void simulateData() {
  // Create dummy JSON for testing UI
  String json = "{";
  json += "\"sys\":{\"date\":\"2025/1/18 (Sun)\",\"time\":\"14:30:05\"},";
  json += "\"cpu\":{\"load\":" + String(random(10, 99)) + ",\"temp\":" + String(random(40, 90)) + "},";
  json += "\"ram\":{\"load\":" + String(random(20, 80)) + ",\"used\":12.5,\"total\":32.0},";
  json += "\"disk\":{\"read\":" + String(random(0, 200)) + ",\"write\":" + String(random(0, 100)) + "},";
  json += "\"net\":{\"dl\":" + String(random(0, 500)/10.0) + ",\"ul\":" + String(random(0, 100)/10.0) + "}";
  json += "}";
  parseAndDisplay(json, false);
}

void drawStaticLayout() {
  tft.fillScreen(C_BG);

  // Top Bar
  tft.fillRoundRect(4, 4, 70, 25, 4, C_PANEL);   // Date
  tft.fillRoundRect(78, 4, 50, 25, 4, C_PANEL);  // Day
  tft.fillRoundRect(132, 4, 108, 25, 4, C_PANEL);// Time (Compact)
  tft.fillRoundRect(244, 4, 72, 25, 4, C_PANEL); // Status (Remaining width)

  // 4 Columns: CPU, RAM, Disk, Net
  // Width 75, Gap 4. X: 4, 83, 162, 241
  tft.fillRoundRect(4, 33, 75, 133, 4, C_PANEL);   // CPU
  tft.fillRoundRect(83, 33, 75, 133, 4, C_PANEL);  // RAM
  tft.fillRoundRect(162, 33, 75, 133, 4, C_PANEL); // Disk
  tft.fillRoundRect(241, 33, 75, 133, 4, C_PANEL); // Net

  // Labels
  tft.setTextColor(C_LABEL, C_PANEL);
  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM);
  
  tft.drawString("CPU", 41, 37);
  tft.drawString("RAM", 120, 37);
  tft.drawString("DISK", 199, 37);
  tft.drawString("NET", 278, 37);

  tft.setTextDatum(TL_DATUM); // Reset
}

void parseAndDisplay(String& json, bool isDataValid) {
  JsonDocument doc; // ArduinoJson v7 handles size automatically
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    // Optional: Print error to serial for debugging
    // Serial.print("JSON Error: "); Serial.println(error.c_str());
    return;
  }

  // --- 1. Status Bar ---
  String fullDate = String((const char*)doc["sys"]["date"]);
  String timeStr = String((const char*)doc["sys"]["time"]);

  // Parse Date & Day
  int spaceIdx = fullDate.indexOf(' ');
  String datePart = (spaceIdx > 0) ? fullDate.substring(0, spaceIdx) : fullDate;
  String dayPart = (spaceIdx > 0) ? fullDate.substring(spaceIdx + 1) : "";

  // Format Date: YYYY/M/D -> MM/DD
  int firstSlash = datePart.indexOf('/');
  int lastSlash = datePart.lastIndexOf('/');
  String month = datePart.substring(firstSlash + 1, lastSlash);
  String day = datePart.substring(lastSlash + 1);
  if (month.length() == 1) month = "0" + month;
  if (day.length() == 1) day = "0" + day;
  String mmdd = month + "/" + day;

  // Format Day: (Sun) -> SUN
  dayPart.replace("(", "");
  dayPart.replace(")", "");
  dayPart.toUpperCase();

  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);

  static String lastDate = "";
  static String lastDay = "";

  if (mmdd != lastDate) {
    tft.setTextPadding(70);
    tft.drawString(mmdd, 39, HEADER_STRING_Y); 
    lastDate = mmdd;
  }
  if (dayPart != lastDay) {
    tft.setTextPadding(50);
    tft.drawString(dayPart, 103, HEADER_STRING_Y); 
    lastDay = dayPart;
  }

  tft.setTextPadding(108);
  tft.drawString(timeStr, 186, HEADER_STRING_Y); 
  tft.setTextPadding(0);

  // Status Icon
  uint16_t statusColor = isDataValid ? C_OK : C_WARN;
  tft.fillCircle(280, 16, 5, statusColor); 
  
  tft.setTextDatum(TL_DATUM); // Reset

  // --- Data Parsing ---
  float cpuLoad = doc["cpu"]["load"];
  int cpuTemp = doc["cpu"]["temp"];
  float ramLoad = doc["ram"]["load"];
  float ramUsed = doc["ram"]["used"];
  float diskR = doc["disk"]["read"] | 0.0;
  float diskW = doc["disk"]["write"] | 0.0;
  float netDL = doc["net"]["dl"];
  float netUL = doc["net"]["ul"];

  // Update Histories
  updateHistory(cpuHistory, cpuLoad);
  updateHistory(ramHistory, ramLoad);
  updateHistory(diskReadHistory, diskR);
  updateHistory(diskWriteHistory, diskW);
  updateHistory(netDlHistory, netDL);
  updateHistory(netUlHistory, netUL);

  // --- 2. CPU Column (x=4) ---
  // Load
  tft.setTextColor(C_CPU, C_PANEL);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(3);
  tft.fillRect(4, 55, 75, 25, C_PANEL);
  tft.drawNumber((int)cpuLoad, 41, 55);
  tft.setTextSize(1);
  tft.drawString("%", 65, 65);
  
  // Temp
  uint16_t tempColor = (cpuTemp > 80) ? C_WARN : C_TEXT;
  tft.setTextColor(tempColor, C_PANEL);
  tft.setTextSize(2);
  tft.fillRect(4, 85, 75, 20, C_PANEL);
  tft.drawNumber(cpuTemp, 41, 85);
  tft.drawString("C", 65, 85);

  // Graph
  drawGraph(largeSprite, cpuHistory, 6, 110, C_CPU, 100);

  // --- 3. RAM Column (x=83) ---
  // Load
  tft.setTextColor(C_RAM, C_PANEL);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(3);
  tft.fillRect(83, 55, 75, 25, C_PANEL);
  tft.drawNumber((int)ramLoad, 120, 55);
  tft.setTextSize(1);
  tft.drawString("%", 144, 65);

  // Used
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(2);
  tft.fillRect(83, 85, 75, 20, C_PANEL);
  tft.drawString(String(ramUsed, 1), 120, 85);
  tft.setTextSize(1);
  tft.drawString("GB", 148, 92);

  // Graph
  drawGraph(largeSprite, ramHistory, 85, 110, C_RAM, 100);

  // --- 4. Disk Column (x=162) ---
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(1);
  
  // Read
  tft.fillRect(164, 50, 71, 15, C_PANEL);
  tft.drawString("R: " + String(diskR, 1), 164, 50);
  drawGraph(smallSprite, diskReadHistory, 164, 65, C_DISK, 0);

  // Write
  tft.fillRect(164, 102, 71, 15, C_PANEL);
  tft.drawString("W: " + String(diskW, 1), 164, 102);
  drawGraph(smallSprite, diskWriteHistory, 164, 117, C_DISK, 0);

  // --- 5. Net Column (x=241) ---
  // DL
  tft.fillRect(243, 50, 71, 15, C_PANEL);
  tft.drawString("D: " + String(netDL, 1), 243, 50);
  drawGraph(smallSprite, netDlHistory, 243, 65, C_NET, 0);

  // UL
  tft.fillRect(243, 102, 71, 15, C_PANEL);
  tft.drawString("U: " + String(netUL, 1), 243, 102);
  drawGraph(smallSprite, netUlHistory, 243, 117, C_NET, 0);
}

void updateHistory(float* history, float value) {
  for (int i = 0; i < GRAPH_W - 1; i++) {
    history[i] = history[i + 1];
  }
  history[GRAPH_W - 1] = value;
}

void drawGraph(TFT_eSprite &sprite, float* history, int x, int y, uint16_t color, float maxValOverride) {
  int w = sprite.width();
  int h = sprite.height();
  
  sprite.fillSprite(C_PANEL);
  sprite.drawFastHLine(0, h/2, w, C_GRID);

  float maxVal = 1.0;
  if (maxValOverride > 0) {
    maxVal = maxValOverride;
  } else {
    for(int i=0; i<GRAPH_W; i++) if(history[i] > maxVal) maxVal = history[i];
  }

  for (int i = 0; i < GRAPH_W - 1; i++) {
    int y1 = map((long)(history[i] * 10), 0, (long)(maxVal * 10), h - 1, 0);
    int y2 = map((long)(history[i + 1] * 10), 0, (long)(maxVal * 10), h - 1, 0);
    // Clamp
    if (y1 < 0) y1 = 0; if (y1 >= h) y1 = h - 1;
    if (y2 < 0) y2 = 0; if (y2 >= h) y2 = h - 1;
    
    sprite.drawLine(i, y1, i + 1, y2, color);
  }
  sprite.pushSprite(x, y);
}