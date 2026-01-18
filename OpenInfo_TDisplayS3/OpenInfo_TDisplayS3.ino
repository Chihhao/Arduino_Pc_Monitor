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

// Objects
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite graphSprite = TFT_eSprite(&tft);
TFT_eSprite netSprite = TFT_eSprite(&tft);

// Global Variables for Graph
#define GRAPH_W 146
#define GRAPH_H 77  // Adjusted height
#define GRAPH_X 8   // Panel X(4) + Padding(4)
#define GRAPH_Y 85  // Moved down slightly
int cpuHistory[GRAPH_W];

#define NET_GRAPH_W 146
#define NET_GRAPH_H 10
float dlHistory[NET_GRAPH_W];
float ulHistory[NET_GRAPH_W];

unsigned long lastDataTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize Screen
  tft.init();
  tft.setRotation(1); // Landscape (USB right) or 3 (USB left)
  tft.fillScreen(C_BG);
  tft.setTextDatum(TL_DATUM);

  // Initialize Sprite for flicker-free graph
  graphSprite.createSprite(GRAPH_W, GRAPH_H);
  netSprite.createSprite(NET_GRAPH_W, NET_GRAPH_H);
  
  // Initialize History Buffer
  for(int i=0; i<GRAPH_W; i++) cpuHistory[i] = 0;
  for(int i=0; i<NET_GRAPH_W; i++) { dlHistory[i] = 0; ulHistory[i] = 0; }

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

  // 1. CPU Panel (Left)
  tft.fillRoundRect(4, 33, 154, 133, 4, C_PANEL);
  
  // 2. RAM Panel (Top Right)
  tft.fillRoundRect(162, 33, 154, 64, 4, C_PANEL);
  
  // 3. NET Panel (Bottom Right)
  tft.fillRoundRect(162, 101, 154, 65, 4, C_PANEL);

  // Labels
  tft.setTextColor(C_LABEL, C_PANEL);
  tft.setTextSize(1);
  
  // CPU Area
  // Split into two halves: Left Center ~42, Right Center ~119 (Panel W=154)
  tft.setTextDatum(TC_DATUM);
  tft.drawString("CPU LOAD", 42, 35);
  tft.drawString("TEMP", 119, 35);
  tft.setTextDatum(TL_DATUM); // Reset
  
  // RAM Area
  tft.drawString("RAM USAGE", 166, 35);
  
  // NET Area
  tft.drawString("NETWORK", 166, 103);
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
    tft.drawString(mmdd, 39, 16); // Center of 4+70
    lastDate = mmdd;
  }
  if (dayPart != lastDay) {
    tft.setTextPadding(50);
    tft.drawString(dayPart, 103, 16); // Center of 78+50
    lastDay = dayPart;
  }

  tft.setTextPadding(108);
  tft.drawString(timeStr, 186, 16); // Center of 132+108
  tft.setTextPadding(0);

  // Status Icon
  uint16_t statusColor = isDataValid ? C_OK : C_WARN;
  tft.fillCircle(280, 16, 5, statusColor); // Center of 244+72
  
  tft.setTextDatum(TL_DATUM); // Reset

  // --- 2. CPU Section ---
  float cpuLoad = doc["cpu"]["load"];
  if (cpuLoad > 99) cpuLoad = 99; // Limit to 2 digits
  int cpuTemp = doc["cpu"]["temp"];
  
  // CPU Load (Big Font)
  tft.setTextSize(3);
  tft.setTextColor(C_ACCENT, C_PANEL);
  tft.setTextDatum(TC_DATUM); // Center numbers
  tft.fillRect(4, 50, 77, 25, C_PANEL); // Clear Left Half
  tft.drawNumber((int)cpuLoad, 42, 50);
  
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("%", 60, 57); // Manual offset

  // CPU Temp
  uint16_t tempColor = (cpuTemp > 80) ? C_WARN : C_TEXT;
  tft.setTextColor(tempColor, C_PANEL);
  tft.setTextSize(3);
  tft.setTextDatum(TC_DATUM);
  tft.fillRect(81, 50, 77, 25, C_PANEL); // Clear Right Half
  tft.drawNumber(cpuTemp, 119, 50);
  // Draw Degree Symbol (Small Circle)
  tft.drawCircle(140, 52, 2, tempColor); 
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("C", 146, 57);

  // CPU Graph Update
  updateGraph(cpuLoad);

  // --- 3. RAM Section ---
  float ramLoad = doc["ram"]["load"];
  if (ramLoad > 99) ramLoad = 99; // Limit to 2 digits
  float ramUsed = doc["ram"]["used"];
  float ramTotal = doc["ram"]["total"];

  // RAM Load %
  tft.setTextSize(3);
  tft.setTextColor(C_ACCENT, C_PANEL);
  tft.fillRect(166, 50, 60, 25, C_PANEL); // Clear
  tft.drawNumber((int)ramLoad, 166, 50);
  tft.setTextSize(2);
  tft.drawString("%", 204, 57); // Moved closer

  // RAM Details (Used/Total)
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT, C_PANEL);
  String ramDetail = String(ramUsed, 1) + " / " + String(ramTotal, 0) + " GB";
  tft.fillRect(230, 50, 70, 20, C_PANEL); // Clear
  tft.drawString(ramDetail, 230, 60);

  // RAM Progress Bar
  int barW = 146;
  int barH = 6;
  int barX = 166;
  int barY = 87;
  int fillW = (int)(barW * (ramLoad / 100.0));
  
  tft.drawRect(barX, barY, barW, barH, C_GRID); // Border
  tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, C_PANEL); // Clear previous
  tft.fillRect(barX + 1, barY + 1, fillW - 2, barH - 2, C_ACCENT); // Fill

  // --- 4. Network Section ---
  float netDL = doc["net"]["dl"];
  float netUL = doc["net"]["ul"];

  // Update History
  for (int i = 0; i < NET_GRAPH_W - 1; i++) {
    dlHistory[i] = dlHistory[i + 1];
    ulHistory[i] = ulHistory[i + 1];
  }
  dlHistory[NET_GRAPH_W - 1] = netDL;
  ulHistory[NET_GRAPH_W - 1] = netUL;

  tft.setTextColor(C_TEXT, C_PANEL);
  
  // Clear area before drawing (using fillRect is safer for variable width fonts)
  tft.fillRect(166, 113, 146, 51, C_PANEL); // Clear NET area
  
  // Draw with mixed font sizes to fit screen
  tft.setTextSize(1); tft.drawString("DL", 166, 114);
  tft.setTextSize(2); tft.drawString(String(netDL, 1) + " MB/s", 188, 109);
  drawNetGraph(dlHistory, 166, 126, C_ACCENT);
  
  tft.setTextSize(1); tft.drawString("UL", 166, 138);
  tft.setTextSize(2); tft.drawString(String(netUL, 1) + " MB/s", 188, 133);
  drawNetGraph(ulHistory, 166, 150, 0xFDA0); // Orange for UL
}

void updateGraph(float value) {
  // Shift history
  for (int i = 0; i < GRAPH_W - 1; i++) {
    cpuHistory[i] = cpuHistory[i + 1];
  }
  cpuHistory[GRAPH_W - 1] = (int)value;

  // Draw Sprite
  graphSprite.fillSprite(C_PANEL); // Match panel color
  
  // Draw Grid lines in sprite
  // graphSprite.drawFastHLine(0, 0, GRAPH_W, C_GRID);
  graphSprite.drawFastHLine(0, GRAPH_H/2, GRAPH_W, C_GRID); // Middle line
  // graphSprite.drawFastHLine(0, GRAPH_H-1, GRAPH_W, C_GRID);

  // Plot Line
  for (int i = 0; i < GRAPH_W - 1; i++) {
    // Map 0-100% to Height (Invert Y because 0 is top)
    int y1 = map(cpuHistory[i], 0, 100, GRAPH_H - 1, 0);
    int y2 = map(cpuHistory[i + 1], 0, 100, GRAPH_H - 1, 0);
    graphSprite.drawLine(i, y1, i + 1, y2, C_ACCENT);
  }

  // Push Sprite to screen
  graphSprite.pushSprite(GRAPH_X, GRAPH_Y);
}

void drawNetGraph(float* history, int x, int y, uint16_t color) {
  netSprite.fillSprite(C_PANEL);
  
  float maxVal = 1.0;
  for(int i=0; i<NET_GRAPH_W; i++) if(history[i] > maxVal) maxVal = history[i];
  
  for (int i = 0; i < NET_GRAPH_W - 1; i++) {
    int y1 = map((long)(history[i] * 10), 0, (long)(maxVal * 10), NET_GRAPH_H - 1, 0);
    int y2 = map((long)(history[i + 1] * 10), 0, (long)(maxVal * 10), NET_GRAPH_H - 1, 0);
    netSprite.drawLine(i, y1, i + 1, y2, color);
  }
  netSprite.pushSprite(x, y);
}