/*
  專案：Arduino 電腦監控 (T-Display S3 版本)
  硬體：LilyGO T-Display S3 (ESP32-S3)
  函式庫需求：
    1. TFT_eSPI (作者：Bodmer)
       *** 設定說明 ***
       請編輯 Arduino libraries/TFT_eSPI/User_Setup_Select.h：
       1. 註解掉：#include <User_Setup.h>
       2. 取消註解：#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>
    2. ArduinoJson (作者：Benoit Blanchon) - 建議使用 v7.x
*/

#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <SPI.h>

// --- 設定 ---
#define SCREEN_W 320
#define SCREEN_H 170

// 顏色
#define C_BG TFT_BLACK
#define C_PANEL 0x18E3 // 深灰
#define C_TEXT TFT_WHITE
#define C_LABEL 0x9CD3 // 淺灰
#define C_ACCENT 0x067F // 青色
#define C_WARN 0xF800   // 紅色
#define C_OK 0x07E0     // 綠色
#define C_GRID 0x2124   // 較深的灰色

// 統一色系設定
#define C_THEME 0xCE40  // 主色調：暗黃色
#define C_CPU C_THEME
#define C_RAM C_THEME
#define C_DISK C_THEME
#define C_NET C_THEME

// 物件
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite largeSprite = TFT_eSprite(&tft);
TFT_eSprite smallSprite = TFT_eSprite(&tft);

#define P_MARGIN 2
#define P_GAP 4
#define PANEL_WIDTH 76
#define PANEL_WIDTH_SMALL 36

// 圖表全域變數
#define GRAPH_W 72      // 76px 面板 - 4px 內距 (每邊 2px)
#define GRAPH_H_L 54    // 大圖表高度 (CPU/RAM)
#define GRAPH_H_S 36    // 小圖表高度 (Disk/Net)

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
  
  // 初始化螢幕
  tft.init();
  tft.setRotation(1); // 橫向 (USB 在右) 或 3 (USB 在左)
  tft.fillScreen(C_BG);
  tft.setTextDatum(TL_DATUM);

  // 初始化 Sprite 以防止圖表閃爍
  largeSprite.createSprite(GRAPH_W, GRAPH_H_L);
  smallSprite.createSprite(GRAPH_W, GRAPH_H_S);
  
  // 初始化歷史緩衝區
  for(int i=0; i<GRAPH_W; i++) {
    cpuHistory[i] = 0; ramHistory[i] = 0;
    diskReadHistory[i] = 0; diskWriteHistory[i] = 0;
    netDlHistory[i] = 0; netUlHistory[i] = 0;
  }

  // 繪製靜態介面佈局
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
    // 模擬模式：若 1 秒無數據，產生假數據
    if (millis() - lastDataTime > 1000) {
      simulateData();
      lastDataTime = millis();
    }
  }
}

void simulateData() {
  // 建立測試 UI 用的假 JSON
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

  // 頂部欄
  // 5 個區塊：76, 76, 76, 36, 36。間距：4
  int x = P_MARGIN;
  tft.fillRoundRect(x, yTop, PANEL_WIDTH, hTop, 4, C_PANEL); x += PANEL_WIDTH + gap; // 日期
  tft.fillRoundRect(x, yTop, PANEL_WIDTH, hTop, 4, C_PANEL); x += PANEL_WIDTH + gap; // 星期
  tft.fillRoundRect(x, yTop, PANEL_WIDTH, hTop, 4, C_PANEL); x += PANEL_WIDTH + gap; // 時間
  tft.fillRoundRect(x, yTop, PANEL_WIDTH_SMALL, hTop, 4, C_PANEL); x += 36 + gap; // 秒
  tft.fillRoundRect(x, yTop, PANEL_WIDTH_SMALL, hTop, 4, C_PANEL);                // 狀態

  // 4 欄：CPU, RAM, Disk, Net
  // 寬度 76，間距 4。X: 2, 82, 162, 242
  int yMain = yTop + hTop + gap;
  int hMain = SCREEN_H - yMain - P_MARGIN; // 170 - (2+25+4) - 2 = 137
  int wMain = 76;
  
  x = P_MARGIN;
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // CPU
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // RAM
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // Disk
  tft.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL);                   // Net

  // 標籤
  tft.setTextColor(C_LABEL, C_PANEL);
  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM); // 置中
  
  // 中心點：2+38=40, 82+38=120, 162+38=200, 242+38=280
  int labelY = yMain + 4;
  tft.drawString("CPU", 40, labelY);
  tft.drawString("RAM", 120, labelY);
  tft.drawString("DISK", 200, labelY);
  tft.drawString("NETWORK", 280, labelY);

  tft.setTextDatum(TL_DATUM); // 重置
}

void parseAndDisplay(String& json, bool isDataValid) {
  JsonDocument doc; // ArduinoJson v7 自動處理大小
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return;
  }

  // --- 1. 狀態欄 ---
  String fullDate = String((const char*)doc["sys"]["date"]);
  String timeStr = String((const char*)doc["sys"]["time"]);

  // 解析日期與星期
  int spaceIdx = fullDate.indexOf(' ');
  String datePart = (spaceIdx > 0) ? fullDate.substring(0, spaceIdx) : fullDate;
  String dayPart = (spaceIdx > 0) ? fullDate.substring(spaceIdx + 1) : "";

  // 格式化日期：YYYY/M/D -> MM/DD
  int firstSlash = datePart.indexOf('/');
  int lastSlash = datePart.lastIndexOf('/');
  String month = datePart.substring(firstSlash + 1, lastSlash);
  String day = datePart.substring(lastSlash + 1);
  if (month.length() == 1) month = "0" + month;
  if (day.length() == 1) day = "0" + day;
  String mmdd = month + "/" + day;

  // 格式化星期：(Sun) -> SUN
  dayPart.replace("(", "");
  dayPart.replace(")", "");
  dayPart.toUpperCase();

  // 分割時間
  String hhmm = (timeStr.length() >= 5) ? timeStr.substring(0, 5) : timeStr;
  String ss = (timeStr.length() >= 8) ? timeStr.substring(6) : "";

  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);

  static String lastDate = "";
  static String lastDay = "";

  if (mmdd != lastDate) {
    tft.setTextPadding(70);
    tft.drawString(mmdd, 40, HEADER_STRING_Y); 
    lastDate = mmdd;
  }
  if (dayPart != lastDay) {
    tft.setTextPadding(70);
    tft.drawString(dayPart, 120, HEADER_STRING_Y); 
    lastDay = dayPart;
  }

  tft.setTextPadding(70);
  tft.drawString(hhmm, 200, HEADER_STRING_Y);
  
  tft.setTextPadding(30);
  tft.drawString(ss, 260, HEADER_STRING_Y);
  tft.setTextPadding(0);

  // 狀態圖示
  uint16_t statusColor = isDataValid ? C_OK : C_WARN;
  tft.fillCircle(300, 15, 5, statusColor); 
  
  tft.setTextDatum(TL_DATUM); // 置左

  // --- 資料解析 ---
  float cpuLoad = doc["cpu"]["load"];
  int cpuTemp = doc["cpu"]["temp"];
  float ramLoad = doc["ram"]["load"];
  float ramUsed = doc["ram"]["used"];
  float diskR = doc["disk"]["read"] | 0.0;
  float diskW = doc["disk"]["write"] | 0.0;
  float netDL = doc["net"]["dl"];
  float netUL = doc["net"]["ul"];

  // 更新歷史紀錄
  updateHistory(cpuHistory, cpuLoad);
  updateHistory(ramHistory, ramLoad);
  updateHistory(diskReadHistory, diskR);
  updateHistory(diskWriteHistory, diskW);
  updateHistory(netDlHistory, netDL);
  updateHistory(netUlHistory, netUL);

  // 用來計算第二排文字的位置
  int _x, _y;

  // --- 2. CPU 欄 (x=2, w=76) ---
  // 負載
  if((int)cpuLoad > 99) cpuLoad = 99;
  
  // 判斷是否需要警示 (負載 > 90% 變紅)
  uint16_t cpuColor = C_CPU;

  _y = 53;
  _x = 55; // 數字與單位的分隔位置
  tft.setTextColor(cpuColor, C_PANEL);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(3);
  tft.fillRect(2, _y, PANEL_WIDTH, 25, C_PANEL); // TODO: 確認清除區域是否正確
  tft.drawNumber((int)cpuLoad, _x, _y);
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("%", _x, _y + 8);
  
  // 分隔線
  tft.drawFastHLine(4, _y + 30, PANEL_WIDTH-4, C_GRID);

  // 溫度
  uint16_t tempColor = (cpuTemp > 80) ? C_WARN : C_TEXT;
  _y = 90;
  _x = 48; // 數字與單位的分隔位置
  tft.setTextColor(tempColor, C_PANEL);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(2);
  tft.fillRect(2, _y, 76, 20, C_PANEL);  // TODO: 確認清除區域是否正確
  tft.drawNumber(cpuTemp, _x - 3, _y);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("C", _x + 5, _y);          
  tft.drawCircle(_x, _y + 2 , 2, tempColor); 

  // 圖表
  drawGraph(largeSprite, cpuHistory, 4, 110, cpuColor, 100);

  // --- 3. RAM 欄 (x=82, w=76) ---
  // 負載
  if((int)ramLoad > 99) ramLoad = 99;

  // 判斷是否需要警示 (負載 > 90% 變紅)
  uint16_t ramColor = C_RAM;

  _y = 53;
  _x = 135; // 82 + 53
  tft.setTextColor(ramColor, C_PANEL);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(3);
  tft.fillRect(82, _y, PANEL_WIDTH, 25, C_PANEL);
  tft.drawNumber((int)ramLoad, _x, _y);
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("%", _x, _y + 8);

  // 分隔線
  tft.drawFastHLine(84, _y + 30, PANEL_WIDTH-4, C_GRID);

  // 已用 RAM
  _y = 90;
  _x = 138; 
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(2);
  tft.fillRect(82, _y, 76, 20, C_PANEL);
  tft.drawFloat(ramUsed, 1, _x, _y);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("GB", _x + 2, _y + 8);

  // 圖表
  drawGraph(largeSprite, ramHistory, 84, 110, ramColor, 100);

  // --- 4. Disk 欄 (x=162, w=76) ---
  // 中心 X = 162 + 38 = 200
  tft.setTextColor(C_TEXT, C_PANEL);
  
  // 讀取
  drawMetric(diskR, 164, 50, false);
  drawGraph(smallSprite, diskReadHistory, 164, 68, C_DISK, 0);

  // 寫入
  drawMetric(diskW, 164, 110, true);
  drawGraph(smallSprite, diskWriteHistory, 164, 128, C_DISK, 0);

  // --- 5. Net 欄 (x=242, w=76) ---
  // 中心 X = 242 + 38 = 280
  // 下載
  drawMetric(netDL, 244, 50, false);
  drawGraph(smallSprite, netDlHistory, 244, 68, C_NET, 0);

  // 上傳
  drawMetric(netUL, 244, 110, true);
  drawGraph(smallSprite, netUlHistory, 244, 128, C_NET, 0);
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
    // 限制範圍
    if (y1 < 0) y1 = 0; if (y1 >= h) y1 = h - 1;
    if (y2 < 0) y2 = 0; if (y2 >= h) y2 = h - 1;
    
    sprite.drawLine(i, y1, i + 1, y2, color);
  }
  sprite.pushSprite(x, y);
}

void drawMetric(float valMB, int x, int y, bool isUp) {
  tft.fillRect(x, y, 72, 18, C_PANEL);
  
  int valInt = 0;
  String unit = "";
  
  if (valMB >= 1024.0) {
    valInt = (int)(valMB / 1024.0 + 0.5);
    unit = "G/s";
  } else if (valMB < 1.0) {
    valInt = (int)(valMB * 1024.0 + 0.5);
    unit = "K/s";
  } else {
    valInt = (int)(valMB + 0.5);
    unit = "M/s";
  }
  
  tft.setTextColor(C_TEXT, C_PANEL);
  
  // 圖示
  int ix = x + 2;
  int iy = y + 4; 
  int _width = 8;
  int _height = 8;

  if (isUp) {
    tft.fillTriangle(ix + _width/2, iy, 
                     ix , iy + _height, 
                     ix + _width, iy + _height, 
                     C_TEXT);
  } else {
    // 倒三角形
    tft.fillTriangle(ix , iy, 
                     ix + _width, iy, 
                     ix + _width/2, iy + _height, 
                     C_TEXT);
  }
  
  // 數字
  tft.setTextSize(2);
  tft.setTextDatum(TR_DATUM); // 置右
  tft.drawNumber(valInt, x + 52, y); 
  
  // 單位
  tft.setTextSize(1); 
  tft.setTextDatum(TL_DATUM); // 置左
  tft.drawString(unit, x + 54, y + 8); 
}