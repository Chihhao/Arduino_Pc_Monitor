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

#define P_MARGIN 2
#define P_GAP 4

// Global Variables for Graph
#define GRAPH_W 72      // 76px Panel - 4px Padding (2px each side)
#define GRAPH_H_L 56    // Large Graph Height (CPU/RAM)
#define GRAPH_H_S 38    // Small Graph Height (Disk/Net)

float cpuHistory[GRAPH_W];
float ramHistory[GRAPH_W];
float diskReadHistory[GRAPH_W];
float diskWriteHistory[GRAPH_W];
float netDlHistory[GRAPH_W];
float netUlHistory[GRAPH_W];

#define HEADER_STRING_Y 15

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
  json += "\"cpu\":{\"load\":" + String(random(0, 101)) + ",\"temp\":" + String(random(0, 101)) + "},";
  json += "\"ram\":{\"load\":" + String(random(0, 101)) + ",\"used\":22.5,\"total\":32.0},";
  json += "\"disk\":{\"read\":" + String(random(0, 101)) + ",\"write\":" + String(random(0, 101)) + "},";
  json += "\"net\":{\"dl\":" + String(random(0, 1001)/10.0) + ",\"ul\":" + String(random(0, 1001)/10.0) + "}";
  json += "}";
  parseAndDisplay(json, false);
}

void drawStaticLayout() {
  tft.fillScreen(C_BG);

  int yTop = P_MARGIN;
  int hTop = 25;
  int gap = P_GAP;

  // Top Bar
  // 320 - 2(margin)*2 - 4(gap)*3 = 304 width available.
  // Distribute: 71, 52, 108, 73
  int x = P_MARGIN;
  tft.fillRoundRect(x, yTop, 71, hTop, 4, C_PANEL); x += 71 + gap; // Date
  tft.fillRoundRect(x, yTop, 52, hTop, 4, C_PANEL); x += 52 + gap; // Day
  tft.fillRoundRect(x, yTop, 108, hTop, 4, C_PANEL); x += 108 + gap;// Time
  tft.fillRoundRect(x, yTop, 73, hTop, 4, C_PANEL);                 // Status

  // 4 Columns: CPU, RAM, Disk, Net
  // Width 76, Gap 4. X: 2, 82, 162, 242
  int yMain = yTop + hTop + gap;
  int hMain = SCREEN_H - yMain - P_MARGIN; // 170 - (2+25+4) - 2 = 137
  int wMain = 76;
  
  x = P_MARGIN;
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // CPU
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // RAM
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // Disk
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL);                   // Net

  // Labels
  tft.setTextColor(C_LABEL, C_PANEL);
  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM);
  
  // Centers: 2+38=40, 82+38=120, 162+38=200, 242+38=280
  int labelY = yMain + 4;
  tft.drawString("CPU", 40, labelY);
  tft.drawString("RAM", 120, labelY);
  tft.drawString("DISK", 200, labelY);
  tft.drawString("NET", 280, labelY);

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
    tft.drawString(mmdd, 38, HEADER_STRING_Y); // Center of 2+71
    lastDate = mmdd;
  }
  if (dayPart != lastDay) {
    tft.setTextPadding(50);
    tft.drawString(dayPart, 103, HEADER_STRING_Y); // Center of 77+52
    lastDay = dayPart;
  }

  tft.setTextPadding(108);
  tft.drawString(timeStr, 187, HEADER_STRING_Y); // Center of 133+108
  tft.setTextPadding(0);

  // Status Icon
  uint16_t statusColor = isDataValid ? C_OK : C_WARN;
  tft.fillCircle(282, 15, 5, statusColor); // Center of 245+73
  
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

  // --- 2. CPU Column (x=2, w=76) ---
  // Load
  tft.setTextColor(C_CPU, C_PANEL);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(3);
  tft.fillRect(2, 51, 76, 25, C_PANEL);
  tft.drawNumber((int)cpuLoad, 40, 51);
  tft.setTextSize(2);
  tft.drawString("%", 65, 61);
  
  // Temp
  uint16_t tempColor = (cpuTemp > 80) ? C_WARN : C_TEXT;
  tft.setTextColor(tempColor, C_PANEL);
  tft.setTextSize(2);
  tft.fillRect(2, 81, 76, 20, C_PANEL);
  tft.drawNumber(cpuTemp, 40, 81);
  tft.drawCircle(62, 83, 2, tempColor);
  tft.drawString("C", 66, 81);

  // Graph
  drawGraph(largeSprite, cpuHistory, 4, 108, C_CPU, 100);

  // --- 3. RAM Column (x=82, w=76) ---
  // Load
  tft.setTextColor(C_RAM, C_PANEL);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(3);
  tft.fillRect(82, 51, 76, 25, C_PANEL);
  tft.drawNumber((int)ramLoad, 120, 51);
  tft.setTextSize(2);
  tft.drawString("%", 144, 61);

  // Used
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(2);
  tft.fillRect(82, 81, 76, 20, C_PANEL);
  tft.drawString(String(ramUsed, 1), 120, 81);
  tft.setTextSize(1);
  tft.drawString("GB", 148, 88);

  // Graph
  drawGraph(largeSprite, ramHistory, 84, 108, C_RAM, 100);

  // --- 4. Disk Column (x=162, w=76) ---
  // Center X = 162 + 38 = 200
  tft.setTextColor(C_TEXT, C_PANEL);
  
  // Read
  tft.fillRect(164, 45, 72, 18, C_PANEL);
  tft.setTextDatum(TR_DATUM); tft.setTextSize(2);
  tft.drawString(String(diskR, 1), 215, 45);
  tft.setTextDatum(TL_DATUM); tft.setTextSize(1);
  tft.drawString("MB", 217, 50);
  drawGraph(smallSprite, diskReadHistory, 164, 63, C_DISK, 0);

  // Write
  tft.fillRect(164, 103, 72, 18, C_PANEL);
  tft.setTextDatum(TR_DATUM); tft.setTextSize(2);
  tft.drawString(String(diskW, 1), 215, 103);
  tft.setTextDatum(TL_DATUM); tft.setTextSize(1);
  tft.drawString("MB", 217, 108);
  drawGraph(smallSprite, diskWriteHistory, 164, 121, C_DISK, 0);

  // --- 5. Net Column (x=242, w=76) ---
  // Center X = 242 + 38 = 280
  // DL
  tft.fillRect(244, 45, 72, 18, C_PANEL);
  tft.setTextDatum(TR_DATUM); tft.setTextSize(2);
  tft.drawString(String(netDL, 1), 295, 45);
  tft.setTextDatum(TL_DATUM); tft.setTextSize(1);
  tft.drawString("MB", 297, 50);
  drawGraph(smallSprite, netDlHistory, 244, 63, C_NET, 0);

  // UL
  tft.fillRect(244, 103, 72, 18, C_PANEL);
  tft.setTextDatum(TR_DATUM); tft.setTextSize(2);
  tft.drawString(String(netUL, 1), 295, 103);
  tft.setTextDatum(TL_DATUM); tft.setTextSize(1);
  tft.drawString("MB", 297, 108);
  drawGraph(smallSprite, netUlHistory, 244, 121, C_NET, 0);
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
  sprite.drawFastHLine(0, 0, w, C_GRID);
  sprite.drawFastHLine(0, h/2, w, C_GRID);
  sprite.drawFastHLine(0, h-1, w, C_GRID);

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