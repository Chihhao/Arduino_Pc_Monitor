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
TFT_eSprite bgSprite = TFT_eSprite(&tft); // 新增全螢幕緩衝區

#define P_MARGIN 2
#define P_GAP 4
#define PANEL_WIDTH 76
#define PANEL_WIDTH_SMALL 36

// 圖表全域變數
#define GRAPH_W 72      // 76px 面板 - 4px 內距 (每邊 2px)
#define GRAPH_H_L 58    // 大圖表高度 (CPU/RAM)
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
  Serial.setRxBufferSize(2048); // 加大接收緩衝區，防止高速數據溢出
  Serial.begin(115200); // 改為標準 115200，對於 10FPS 數據量已足夠且相容性更好 (電腦端需對應修改)
  
  // 初始化螢幕
  tft.init();
  tft.setRotation(1); // 橫向 (USB 在右) 或 3 (USB 在左)
  tft.fillScreen(C_BG);
  tft.setTextDatum(TL_DATUM);

  // 初始化 Sprite 以防止圖表閃爍
  bgSprite.createSprite(SCREEN_W, SCREEN_H); // 建立全螢幕緩衝區
  largeSprite.createSprite(GRAPH_W, GRAPH_H_L);
  smallSprite.createSprite(GRAPH_W, GRAPH_H_S);
  
  // 初始化歷史緩衝區
  for(int i=0; i<GRAPH_W; i++) {
    cpuHistory[i] = 0; ramHistory[i] = 0;
    diskReadHistory[i] = 0; diskWriteHistory[i] = 0;
    netDlHistory[i] = 0; netUlHistory[i] = 0;
  }

  // 移除此處的 drawStaticLayout，改在 parseAndDisplay 中每一幀繪製
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      parseAndDisplay(input, true);
      lastDataTime = millis();
    }
  } else {
    // 模擬模式：改為 100ms (10 FPS)
    if (millis() - lastDataTime > 100) {
      simulateData();
      lastDataTime = millis();
    }
  }
}

void simulateData() {
  static int valCpu = 20;
  static int valTemp = 40;
  static int valRam = 30;
  static float valDiskR = 0;
  static float valDiskW = 0;
  static float valNetDl = 0;
  static float valNetUl = 0;

  // 模擬數據波動 (Random Walk)
  // 為了配合 10 FPS，將變化幅度改小，讓動畫更平滑
  valCpu += random(-2, 3);
  if (valCpu < 0) valCpu = 0; if (valCpu > 100) valCpu = 100;

  // 溫度跟隨負載緩慢變化
  int targetTemp = 35 + (valCpu / 2);
  if (valTemp < targetTemp) valTemp++; else if (valTemp > targetTemp) valTemp--;

  valRam += random(-1, 2);
  if (valRam < 10) valRam = 10; if (valRam > 98) valRam = 98;

  // 網路與硬碟偶爾突波，其餘時間衰退
  if (random(100) > 90) valDiskR = random(5, 80); else valDiskR *= 0.7;
  if (random(100) > 90) valDiskW = random(5, 80); else valDiskW *= 0.7;
  if (random(100) > 85) valNetDl = random(2, 60); else valNetDl *= 0.8;
  if (random(100) > 85) valNetUl = random(1, 20); else valNetUl *= 0.8;

  // 建立測試 UI 用的假 JSON
  String json = "{";
  json += "\"sys\":{\"date\":\"2025/1/18 (Sun)\",\"time\":\"14:30:05\"},";
  json += "\"cpu\":{\"load\":" + String(valCpu) + ",\"temp\":" + String(valTemp) + "},";
  json += "\"ram\":{\"load\":" + String(valRam) + ",\"used\":22.5,\"total\":32.0},";
  json += "\"disk\":{\"read\":" + String(valDiskR, 1) + ",\"write\":" + String(valDiskW, 1) + "},";
  json += "\"net\":{\"dl\":" + String(valNetDl, 1) + ",\"ul\":" + String(valNetUl, 1) + "}";
  json += "}";
  parseAndDisplay(json, false);
}

void drawStaticLayout() {
  bgSprite.fillScreen(C_BG); // 改為清除緩衝區

  int yTop = P_MARGIN;
  int hTop = 25;
  int gap = P_GAP;

  // 頂部欄
  // 5 個區塊：76, 76, 76, 36, 36。間距：4
  int x = P_MARGIN;
  bgSprite.fillRoundRect(x, yTop, PANEL_WIDTH, hTop, 4, C_PANEL); x += PANEL_WIDTH + gap; // 日期
  bgSprite.fillRoundRect(x, yTop, PANEL_WIDTH, hTop, 4, C_PANEL); x += PANEL_WIDTH + gap; // 星期
  bgSprite.fillRoundRect(x, yTop, PANEL_WIDTH, hTop, 4, C_PANEL); x += PANEL_WIDTH + gap; // 時間
  bgSprite.fillRoundRect(x, yTop, PANEL_WIDTH_SMALL, hTop, 4, C_PANEL); x += 36 + gap; // 秒
  bgSprite.fillRoundRect(x, yTop, PANEL_WIDTH_SMALL, hTop, 4, C_PANEL);                // 狀態

  // 4 欄：CPU, RAM, Disk, Net
  // 寬度 76，間距 4。X: 2, 82, 162, 242
  int yMain = yTop + hTop + gap;
  int hMain = SCREEN_H - yMain - P_MARGIN; // 170 - (2+25+4) - 2 = 137
  int wMain = 76;
  
  x = P_MARGIN;
  bgSprite.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // CPU
  bgSprite.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // RAM
  bgSprite.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL); x += wMain + gap; // Disk
  bgSprite.fillRoundRect(x, yMain, wMain, hMain, 4, C_PANEL);                   // Net

  // 標籤
  bgSprite.setTextColor(C_LABEL, C_PANEL);
  bgSprite.setTextSize(1);
  bgSprite.setTextDatum(TC_DATUM); // 置中
  
  // 中心點：2+38=40, 82+38=120, 162+38=200, 242+38=280
  int labelY = yMain + 4;
  bgSprite.drawString("CPU", 40, labelY);
  bgSprite.drawString("RAM", 120, labelY);
  bgSprite.drawString("DISK", 200, labelY);
  bgSprite.drawString("NETWORK", 280, labelY);

  bgSprite.setTextDatum(TL_DATUM); // 重置
}

void parseAndDisplay(String& json, bool isDataValid) {
  JsonDocument doc; // ArduinoJson v7 自動處理大小
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return;
  }

  // --- 每一幀都重繪背景與靜態佈局 (雙重緩衝) ---
  drawStaticLayout();

  // --- 0. 預先解析數據 (供圖表與文字使用) ---
  float cpuLoad = doc["cpu"]["load"];
  int cpuTemp = doc["cpu"]["temp"];
  float ramLoad = doc["ram"]["load"];
  float ramUsed = doc["ram"]["used"];
  float diskR = doc["disk"]["read"] | 0.0;
  float diskW = doc["disk"]["write"] | 0.0;
  float netDL = doc["net"]["dl"];
  float netUL = doc["net"]["ul"];

  // 更新歷史紀錄 (每一幀都更新，保持 10 FPS 流暢度)
  updateHistory(cpuHistory, cpuLoad);
  updateHistory(ramHistory, ramLoad);
  updateHistory(diskReadHistory, diskR);
  updateHistory(diskWriteHistory, diskW);
  updateHistory(netDlHistory, netDL);
  updateHistory(netUlHistory, netUL);

  // --- 1. 文字數據更新 (每秒一次) ---
  // 定義靜態變數以快取文字數據
  static unsigned long lastTextUpdate = 0;
  static String s_mmdd = "--/--";
  static String s_dayPart = "---";
  static String s_hhmm = "--:--";
  static String s_ss = "--";
  static uint16_t s_statusColor = C_WARN;
  static int s_cpuTemp = 0;
  static uint16_t s_tempColor = C_TEXT;
  static int s_cpuLoad = 0;
  static float s_ramUsed = 0;
  static int s_ramLoad = 0;
  static float s_diskR = 0;
  static float s_diskW = 0;
  static float s_netDL = 0;
  static float s_netUL = 0;

  // 每秒更新一次快取變數
  if (millis() - lastTextUpdate > 1000 || lastTextUpdate == 0) {
    lastTextUpdate = millis();

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
    s_mmdd = month + "/" + day;

    // 格式化星期
    dayPart.replace("(", "");
    dayPart.replace(")", "");
    dayPart.toUpperCase();
    s_dayPart = dayPart;

    // 時間
    s_hhmm = (timeStr.length() >= 5) ? timeStr.substring(0, 5) : timeStr;
    s_ss = (timeStr.length() >= 8) ? timeStr.substring(6) : "";

    // 狀態
    s_statusColor = isDataValid ? C_OK : C_WARN;

    // CPU
    s_cpuTemp = cpuTemp;
    s_tempColor = (s_cpuTemp > 80) ? C_WARN : C_TEXT;
    s_cpuLoad = (int)cpuLoad;
    if (s_cpuLoad > 99) s_cpuLoad = 99;

    // RAM
    s_ramUsed = ramUsed;
    s_ramLoad = (int)ramLoad;
    if (s_ramLoad > 99) s_ramLoad = 99;

    // Disk/Net
    s_diskR = diskR;
    s_diskW = diskW;
    s_netDL = netDL;
    s_netUL = netUL;
  }

  // --- 2. 文字繪製 (每一幀都重繪，但使用快取變數) ---
  {
    bgSprite.setTextColor(C_TEXT, C_PANEL);
    bgSprite.setTextSize(2);
    bgSprite.setTextDatum(MC_DATUM);

    bgSprite.setTextPadding(70);
    bgSprite.drawString(s_mmdd, 41, HEADER_STRING_Y); 
    
    bgSprite.setTextPadding(70);
    bgSprite.drawString(s_dayPart, 121, HEADER_STRING_Y); 

    bgSprite.setTextPadding(70);
    bgSprite.drawString(s_hhmm, 201, HEADER_STRING_Y);
    
    bgSprite.setTextPadding(30);
    bgSprite.drawString(s_ss, 261, HEADER_STRING_Y);
    bgSprite.setTextPadding(0);

    // 狀態圖示
    bgSprite.fillCircle(300, 14, 5, s_statusColor); 
    
    bgSprite.setTextDatum(TL_DATUM); // 置左

    // 用來計算第二排文字的位置
    int _x, _y;

    // --- CPU 文字 ---
    // CPU 溫度
    _y = 50;
    _x = 46; 
    bgSprite.setTextColor(s_tempColor, C_PANEL);
    bgSprite.setTextDatum(TR_DATUM);
    bgSprite.setTextSize(2);
    bgSprite.fillRect(2, _y, PANEL_WIDTH, 20, C_PANEL);
    bgSprite.drawNumber(s_cpuTemp, _x - 3, _y);
    bgSprite.setTextDatum(TL_DATUM);
    bgSprite.drawString("C", _x + 5, _y);          
    bgSprite.drawCircle(_x, _y + 2 , 2, s_tempColor); 

    // 分隔線
    bgSprite.drawFastHLine(4, _y + 18, PANEL_WIDTH-4, C_GRID);

    // CPU Load 
    _y = 77;
    _x = 55; 
    bgSprite.setTextColor(C_CPU, C_PANEL);
    bgSprite.setTextDatum(TR_DATUM);
    bgSprite.setTextSize(3);    
    bgSprite.fillRect(2, _y, PANEL_WIDTH, 25, C_PANEL);
    bgSprite.drawNumber(s_cpuLoad, _x, _y);
    bgSprite.setTextSize(2);
    bgSprite.setTextDatum(TL_DATUM);
    bgSprite.drawString("%", _x, _y + 8);

    // --- RAM 文字 ---
    // 已用 RAM
    _y = 50;
    _x = 138; 
    bgSprite.setTextColor(C_TEXT, C_PANEL);
    bgSprite.setTextDatum(TR_DATUM);
    bgSprite.setTextSize(2);    
    bgSprite.fillRect(82, _y, PANEL_WIDTH, 20, C_PANEL);
    bgSprite.drawFloat(s_ramUsed, 1, _x, _y);
    bgSprite.setTextSize(1);
    bgSprite.setTextDatum(TL_DATUM);
    bgSprite.drawString("GB", _x + 2, _y + 8);

    // 分隔線
    bgSprite.drawFastHLine(84, _y + 18, PANEL_WIDTH-4, C_GRID);

    // RAM Load
    _y = 77;
    _x = 135; // 82 + 53
    bgSprite.setTextColor(C_RAM, C_PANEL);
    bgSprite.setTextDatum(TR_DATUM);
    bgSprite.setTextSize(3);

    bgSprite.fillRect(82, _y, PANEL_WIDTH, 25, C_PANEL);

    bgSprite.drawNumber(s_ramLoad, _x, _y);
    bgSprite.setTextSize(2);
    bgSprite.setTextDatum(TL_DATUM);
    bgSprite.drawString("%", _x, _y + 8);

    // --- Disk 文字 ---
    drawMetric(s_diskR, 164, 50, false);
    drawMetric(s_diskW, 164, 110, true);

    // --- Net 文字 ---
    drawMetric(s_netDL, 244, 50, false);
    drawMetric(s_netUL, 244, 110, true);
  }

  // --- 2. 圖表更新 (每一幀都更新) ---
  drawGraph(largeSprite, cpuHistory, 4, 106, C_CPU, 100);
  drawGraph(largeSprite, ramHistory, 84, 106, C_RAM, 100);
  drawGraph(smallSprite, diskReadHistory, 164, 68, C_DISK, 0);
  drawGraph(smallSprite, diskWriteHistory, 164, 128, C_DISK, 0);
  drawGraph(smallSprite, netDlHistory, 244, 68, C_NET, 0);
  drawGraph(smallSprite, netUlHistory, 244, 128, C_NET, 0);

  // 當沒有連線 (模擬模式) 時，在最上層顯示 DEMO 標籤
  if (!isDataValid) {
    int boxW = 80;
    int boxH = 32;
    int boxX = (SCREEN_W - boxW) / 2;
    int boxY = (SCREEN_H - boxH) / 2 + 2;

    // 繪製背景 (黑色) 與邊框 (紅色)
    bgSprite.fillRoundRect(boxX, boxY, boxW, boxH, 6, C_BG);
    bgSprite.drawRoundRect(boxX, boxY, boxW, boxH, 6, C_WARN);

    // 繪製文字
    bgSprite.setTextColor(C_WARN, C_BG);
    bgSprite.setTextSize(2);
    bgSprite.setTextDatum(MC_DATUM);
    bgSprite.drawString("DEMO", SCREEN_W / 2, SCREEN_H / 2);
    bgSprite.setTextDatum(TL_DATUM); // 還原對齊設定
  }

  // --- 最後一次性將緩衝區推送到螢幕，消除閃爍 ---
  bgSprite.pushSprite(0, 0);
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

  // 1. 繪製區域填充 (Area Fill Gradient)
  for (int i = 0; i < GRAPH_W; i++) {
    int valY = map((long)(history[i] * 10), 0, (long)(maxVal * 10), h - 1, 0);
    if (valY < 0) valY = 0; if (valY >= h) valY = h - 1;

    int graphHeight = h - valY;

    // 從數值高度向下畫到底部
    for (int row = valY; row < h; row++) {
       // 優化：直接判斷背景色，不使用 readPixel
       // 格線位置：0, h/2, h-1
       uint16_t bg = (row == 0 || row == h/2 || row == h-1) ? C_GRID : C_PANEL;

       // 計算透明度
       int alpha = 0;
       if (graphHeight > 0) {
         alpha = 180 - (180 * (row - valY) / graphHeight);
       }
       
       // 只有當 alpha > 0 才需要繪製 (避免無謂的寫入)
       if (alpha > 0) {
         uint16_t blended = tft.alphaBlend(alpha, color, bg);
         sprite.drawPixel(i, row, blended);
       }
    }
  }

  // 2. 繪製頂部線條 (Line) - 保持銳利
  for (int i = 0; i < GRAPH_W - 1; i++) {
    int y1 = map((long)(history[i] * 10), 0, (long)(maxVal * 10), h - 1, 0);
    int y2 = map((long)(history[i + 1] * 10), 0, (long)(maxVal * 10), h - 1, 0);
    // 限制範圍
    if (y1 < 0) y1 = 0; if (y1 >= h) y1 = h - 1;
    if (y2 < 0) y2 = 0; if (y2 >= h) y2 = h - 1;
    
    sprite.drawLine(i, y1, i + 1, y2, color);
  }

  // 改為推送到全螢幕緩衝區，而不是直接推送到螢幕
  sprite.pushToSprite(&bgSprite, x, y);
}

void drawMetric(float valMB, int x, int y, bool isUp) {
  bgSprite.fillRect(x, y, 72, 18, C_PANEL);
  
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
  
  bgSprite.setTextColor(C_TEXT, C_PANEL);
  
  // 圖示
  int ix = x + 2;
  int iy = y + 4; 
  int _width = 8;
  int _height = 8;

  if (isUp) {
    bgSprite.fillTriangle(ix + _width/2, iy, 
                     ix , iy + _height, 
                     ix + _width, iy + _height, 
                     C_TEXT);
  } else {
    // 倒三角形
    bgSprite.fillTriangle(ix , iy, 
                     ix + _width, iy, 
                     ix + _width/2, iy + _height, 
                     C_TEXT);
  }
  
  // 數字
  bgSprite.setTextSize(2);
  bgSprite.setTextDatum(TR_DATUM); // 置右
  bgSprite.drawNumber(valInt, x + 52, y); 
  
  // 單位
  bgSprite.setTextSize(1); 
  bgSprite.setTextDatum(TL_DATUM); // 置左
  bgSprite.drawString(unit, x + 54, y + 8); 
}