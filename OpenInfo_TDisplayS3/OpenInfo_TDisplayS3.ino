/*
  專案：Arduino 電腦監控 (T-Display S3 版本)
  硬體：LilyGO T-Display S3 (ESP32-S3)
    1. Board (開發板): ESP32S3 Dev Module
    2. USB CDC On Boot: Enabled 
    3. CPU Frequency: 240MHz (WiFi)
    4. Core Debug Level: None
    5. USB DFU On Boot: Disabled
    6. Erase All Flash Before Sketch Upload: Disabled
    7. Flash Mode: QIO 80MHz
       說明：QIO 是最快的讀寫模式。如果燒錄後無法開機，可改試 DIO 80MHz，但 T-Display S3 通常支援 QIO。
    8. Flash Size: 16MB (128Mb)
       說明：這是此板子的標準規格。
    9. Partition Scheme (分割區配置): 16M Flash (3MB APP/9.9MB FATFS)
    10. PSRAM: OPI PSRAM 

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

#define PIN_BTN 0 // 板載 Boot 按鈕
#define PIN_BTN_ROT 14 // 板載 IO14 按鈕 (左邊那顆，若是 AMOLED 版請改為 21)

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
#define HISTORY_LEN 320 // 擴大歷史紀錄至全螢幕寬度
#define DASH_GRAPH_W 72 // Dashboard 小圖表寬度
#define GRAPH_H_L 58    // 大圖表高度 (CPU/RAM)
#define GRAPH_H_S 36    // 小圖表高度 (Disk/Net)

float cpuHistory[HISTORY_LEN];
float ramHistory[HISTORY_LEN];
float diskReadHistory[HISTORY_LEN];
float diskWriteHistory[HISTORY_LEN];
float netDlHistory[HISTORY_LEN];
float netUlHistory[HISTORY_LEN];

#define HEADER_STRING_Y 15

unsigned long lastDataTime = 0;

void setup() {
  Serial.setRxBufferSize(2048); // 加大接收緩衝區，防止高速數據溢出
  Serial.begin(115200); // 改為標準 115200，對於 10FPS 數據量已足夠且相容性更好 (電腦端需對應修改)
  
  // 等待 Serial 連線，但最多只等 3 秒 (避免沒接電腦時卡住)
  unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart < 3000)) {
    delay(10);
  }

  // Total PSRAM: 8386295
  // Free PSRAM: 8386295
  // Flash Size: 16 MB
  Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
  Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);
  // 預期輸出: Width: 320, Height: 170
  Serial.println("TDISPLAYS3");

  pinMode(PIN_BTN, INPUT_PULLUP); // 初始化按鈕
  pinMode(PIN_BTN_ROT, INPUT_PULLUP); // 初始化旋轉按鈕
  
  // 初始化螢幕
  tft.init();
  tft.setRotation(1); // 改回 1 (USB 在右)
  
  tft.fillScreen(C_BG);
  Serial.printf("TFT Width: %d, Height: %d\n", tft.width(), tft.height());
  tft.setTextDatum(TL_DATUM);

  // 初始化 Sprite 以防止圖表閃爍
  bgSprite.createSprite(SCREEN_W, SCREEN_H); // 建立全螢幕緩衝區
  bgSprite.setSwapBytes(true); // 確保位元組順序正確，防止像素錯亂
  if (!bgSprite.created()) {
    Serial.println("BG Sprite create failed!");
    tft.fillScreen(C_WARN);
    tft.setTextColor(C_TEXT, C_WARN);
    tft.drawString("PSRAM Alloc Failed!", 10, 50);
    tft.drawString("Enable OPI PSRAM", 10, 80);
    while(1) delay(1000); // 停在這裡，避免後續執行錯誤
  }

  largeSprite.createSprite(DASH_GRAPH_W, GRAPH_H_L);
  if (!largeSprite.created()) Serial.println("Large Sprite create failed!");
  smallSprite.createSprite(DASH_GRAPH_W, GRAPH_H_S);
  if (!smallSprite.created()) Serial.println("Small Sprite create failed!");
  
  // 初始化歷史緩衝區
  clearHistory();
}

// 新增變數：追蹤軟體是否主動連線
bool softwareConnected = false;
bool isSleeping = false; // 新增：螢幕休眠狀態
int currentPage = 0; // 0: Dashboard, 1: Clock, 2: Big Graph

void loop() {
  // 如果物理連線中斷，軟體連線也視為中斷
  if (!Serial) softwareConnected = false;

  // 偵測連線狀態變化，並在切換時清空歷史紀錄
  // 現在「線上」的定義是：USB 線插著 (Serial) 且 軟體有在傳資料 (softwareConnected)
  bool isOnline = (bool)Serial && softwareConnected;
  static bool wasOnline = false; 

  if (isOnline != wasOnline) {
    clearHistory();
    wasOnline = isOnline;
  }

  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // 去除前後空白與換行符號

    if (input == "WHO") {
      Serial.println("TDISPLAYS3"); // 回應握手訊號
    } else if (input == "BYE") {
      softwareConnected = false;    // 收到 Python 的結束訊號
    } else if (input == "SLEEP") {
      isSleeping = true;
      tft.fillScreen(TFT_BLACK);    // 立即黑屏
      softwareConnected = true;
    } else if (input == "MODE1") {
      currentPage = 0;
      isSleeping = false;           // 切換模式時喚醒
      softwareConnected = true;
    } else if (input == "MODE2") {
      currentPage = 1;
      isSleeping = false;
      softwareConnected = true;
    } else if (input == "MODE3") {
      currentPage = 2;
      isSleeping = false;
      softwareConnected = true;
    } else if (input == "FLIP") {
      tft.setRotation(tft.getRotation() == 1 ? 3 : 1); // 切換旋轉方向 (1 <-> 3)
      isSleeping = false;
      softwareConnected = true;
    } else if (input.length() > 0) {
      softwareConnected = true;     // 收到有效資料，標記為已連線
      parseAndDisplay(input, true);
      lastDataTime = millis();
    }
  } else {
    // 若無可用資料，則檢查連線狀態來決定下一步
    if (millis() - lastDataTime > 100) { // 以 10FPS 的頻率更新，確保 UI 反應
      if (isOnline) {
        // 連線中但閒置：重繪畫面以處理按鈕事件，但不進入 DEMO
        String dummy = ""; // 傳送空字串，讓解析失敗從而沿用舊數據，避免畫面歸零閃爍
        parseAndDisplay(dummy, true);
      } else {
        // 已斷線：執行 DEMO 模式
        simulateData();
      }
      // 更新時間戳，以維持 10FPS 的更新率
      lastDataTime = millis();
    }
  }
}

void clearHistory() {
  // 使用 memset 歸零，比 for 迴圈更有效率
  memset(cpuHistory, 0, sizeof(cpuHistory));
  memset(ramHistory, 0, sizeof(ramHistory));
  memset(diskReadHistory, 0, sizeof(diskReadHistory));
  memset(diskWriteHistory, 0, sizeof(diskWriteHistory));
  memset(netDlHistory, 0, sizeof(netDlHistory));
  memset(netUlHistory, 0, sizeof(netUlHistory));
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

// 全域變數儲存格式化後的文字，供不同頁面共用
String s_mmdd = "--/--";
String s_dayPart = "---";
String s_hhmm = "--:--";
String s_ss = "--";
String s_year = "----";
String s_month_str = "---";
String s_day_num = "--";
uint16_t s_statusColor = C_WARN;
int s_cpuTemp = 0;
uint16_t s_tempColor = C_TEXT;
int s_cpuLoad = 0;
float s_ramUsed = 0;
int s_ramLoad = 0;
float s_diskR = 0;
float s_diskW = 0;
float s_netDL = 0;
float s_netUL = 0;

// --- 頁面 1: Dashboard 繪圖邏輯 ---
void drawDashboard(bool isDataValid) {
  // 繪製靜態佈局

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

  // 繪製文字
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

  // 圖表更新
  drawGraph(largeSprite, cpuHistory, 4, 106, C_CPU, 100);
  drawGraph(largeSprite, ramHistory, 84, 106, C_RAM, 100);
  drawGraph(smallSprite, diskReadHistory, 164, 68, C_DISK, 0);
  drawGraph(smallSprite, diskWriteHistory, 164, 128, C_DISK, 0);
  drawGraph(smallSprite, netDlHistory, 244, 68, C_NET, 0);
  drawGraph(smallSprite, netUlHistory, 244, 128, C_NET, 0);

  if (!isDataValid) drawDemoBox();
}

void parseAndDisplay(String& json, bool isDataValid) {
  JsonDocument doc; // ArduinoJson v7 自動處理大小
  DeserializationError error = deserializeJson(doc, json);

  // 即使 JSON 解析失敗，也不要直接 return，確保按鈕偵測和畫面重繪(使用舊數據)能持續運作。

  // --- 按鈕處理 (切換頁面) ---
  static unsigned long lastBtnTime = 0;
  static bool btnPressed = false;
  static int lastPage = 0;

  if (digitalRead(PIN_BTN) == LOW) {
    if (!btnPressed && millis() - lastBtnTime > 200) { // 防止連點與彈跳
      currentPage = (currentPage + 1) % 3;
      btnPressed = true;
      lastBtnTime = millis();
      isSleeping = false; // 按鈕喚醒
    }
  } else {
    btnPressed = false;
  }

  // --- 按鈕 2 處理 (旋轉螢幕) ---
  static unsigned long lastBtn2Time = 0;
  static bool btn2Pressed = false;

  if (digitalRead(PIN_BTN_ROT) == LOW) {
    if (!btn2Pressed && millis() - lastBtn2Time > 200) { // 防彈跳
      tft.setRotation(tft.getRotation() == 1 ? 3 : 1); // 切換 1 (USB右) <-> 3 (USB左)
      btn2Pressed = true;
      lastBtn2Time = millis();
      isSleeping = false; // 按鈕喚醒
    }
  } else {
    btn2Pressed = false;
  }

  if (currentPage != lastPage) {
    lastPage = currentPage;
  }

  // --- 0. 預先解析數據 (供圖表與文字使用) ---
  float cpuLoad = 0;
  int cpuTemp = 0;
  float ramLoad = 0;
  float ramUsed = 0;
  float diskR = 0;
  float diskW = 0;
  float netDL = 0;
  float netUL = 0;

  // 只有當 JSON 正確時才更新數據，否則沿用上一幀的數據
  if (!error) {
    cpuLoad = doc["cpu"]["load"];
    cpuTemp = doc["cpu"]["temp"];
    ramLoad = doc["ram"]["load"];
    ramUsed = doc["ram"]["used"];
    diskR = doc["disk"]["read"] | 0.0;
    diskW = doc["disk"]["write"] | 0.0;
    netDL = doc["net"]["dl"];
    netUL = doc["net"]["ul"];

    // 更新歷史紀錄
    updateHistory(cpuHistory, cpuLoad);
    updateHistory(ramHistory, ramLoad);
    updateHistory(diskReadHistory, diskR);
    updateHistory(diskWriteHistory, diskW);
    updateHistory(netDlHistory, netDL);
    updateHistory(netUlHistory, netUL);
  }

  // --- 1. 文字數據更新 (每秒一次) ---
  // 定義靜態變數以快取文字數據
  static unsigned long lastTextUpdate = 0;

  // 每秒更新一次快取變數 (且必須 JSON 有效)
  if (!error && (millis() - lastTextUpdate > 1000 || lastTextUpdate == 0)) {
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
    s_year = datePart.substring(0, firstSlash);
    String month_num_str = datePart.substring(firstSlash + 1, lastSlash);
    s_day_num = datePart.substring(lastSlash + 1);

    // 將月份數字轉為英文縮寫
    switch(month_num_str.toInt()) {
        case 1: s_month_str = "JAN"; break;
        case 2: s_month_str = "FEB"; break;
        case 3: s_month_str = "MAR"; break;
        case 4: s_month_str = "APR"; break;
        case 5: s_month_str = "MAY"; break;
        case 6: s_month_str = "JUN"; break;
        case 7: s_month_str = "JUL"; break;
        case 8: s_month_str = "AUG"; break;
        case 9: s_month_str = "SEP"; break;
        case 10: s_month_str = "OCT"; break;
        case 11: s_month_str = "NOV"; break;
        case 12: s_month_str = "DEC"; break;
        default: s_month_str = "---"; break;
    }
    String month = month_num_str;
    String day = s_day_num;
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

  // 如果處於休眠模式，則不繪製畫面
  if (isSleeping) return;

  // --- 2. 根據頁面繪製 ---
  // 使用 memset 強制歸零記憶體，這是最底層的清除方式，保證舊資料絕對不會留下來 (防止殘留)
  memset(bgSprite.getPointer(), 0, SCREEN_W * SCREEN_H * 2); 
  
  // 再使用 fillRect 填入我們想要的背景色
  bgSprite.fillRect(0, 0, SCREEN_W, SCREEN_H, C_BG);

  if (currentPage == 0) {
    drawDashboard(isDataValid);
  } else if (currentPage == 1) {
    drawClockPage(isDataValid);
  } else {
    drawBigGraphPage(isDataValid);
  }

  // --- 最後一次性將緩衝區推送到螢幕，消除閃爍 ---
  bgSprite.pushSprite(0, 0);
}

// --- 頁面 2: 大時鐘 ---
void drawClockPage(bool isDataValid) {
  // 定義日曆區域 (150x150, Margin 10)
  int margin = 10;
  int gap = 12;
  int calX = margin;
  int calY = margin;
  int calW = 150;
  int calH = 150;

  // 右側區域設定 
  int rightX = margin + calW + gap; 
  int rightW = SCREEN_W - margin - calW - gap - margin;
  int rightCenterX = rightX + rightW / 2 + 2;

  // --- 左側：日曆---
  // 日曆紙背景
  bgSprite.fillRect(calX, calY, calW, calH, 0xE71C);
  
  // 頂部紅色橫條
  bgSprite.fillRect(calX, calY, calW, 28, C_WARN);
  bgSprite.setTextColor(TFT_WHITE, C_WARN);
  bgSprite.setTextSize(2);
  bgSprite.setTextDatum(MC_DATUM);
  bgSprite.drawString(s_month_str + " " + s_year, calX + calW / 2, calY + 15);

  // 中間：巨大日期
  bgSprite.setTextColor(TFT_BLACK, 0xE71C);
  bgSprite.setTextDatum(MC_DATUM);
  bgSprite.setTextSize(10); // 縮小字體以適應 150px 高度
  bgSprite.drawString(s_day_num, calX + calW / 2, calY + 75); 

  // 底部：星期
  bgSprite.setTextColor(TFT_BLACK, 0xE71C); 
  bgSprite.setTextSize(3);
  bgSprite.setTextDatum(BC_DATUM);
  bgSprite.drawString(s_dayPart, calX + calW / 2, calY + calH - 10);

  // --- 右側：時鐘與狀態 ---
  // 時間 (HH:MM) 分割顯示，冒號閃爍
  bgSprite.setTextColor(C_TEXT, C_BG);
  bgSprite.setTextSize(5); 
  
  int timeY = 62; 
  String hh = (s_hhmm.length() >= 2) ? s_hhmm.substring(0, 2) : "--";
  String mm = (s_hhmm.length() >= 5) ? s_hhmm.substring(3, 5) : "--";

  // 時
  bgSprite.setTextDatum(MR_DATUM);
  bgSprite.drawString(hh, rightCenterX - 8, timeY);
  
  // 分
  bgSprite.setTextDatum(ML_DATUM);
  bgSprite.drawString(mm, rightCenterX + 8, timeY);

  // 冒號 (每秒閃爍)
  if (millis() % 1000 < 500) {
    bgSprite.setTextColor(C_TEXT); // 使用透明背景，避免遮擋
    bgSprite.setTextDatum(MC_DATUM);
    bgSprite.drawString(":", rightCenterX, timeY - 2);
  }

  // --- CPU / RAM 使用率---
  int barY_second = 16;
  int barY_cpu = 116;
  int barY_ram = 150;
  int barX = rightX;
  int barW = rightW;
  int barH = 6;

  // 秒數進度條
  bgSprite.drawRoundRect(barX, barY_second, barW, barH, 2, C_GRID);
  int seconds = s_ss.toInt();
  int secBarW = map(seconds, 0, 60, 0, barW);
  if (secBarW > 0) {
    bgSprite.fillRoundRect(barX, barY_second, secBarW, barH, 2, C_ACCENT);
  }

  // CPU
  bgSprite.setTextColor(C_CPU, C_BG);
  bgSprite.setTextSize(2);
  bgSprite.setTextDatum(TL_DATUM);
  bgSprite.drawString("CPU", barX, barY_cpu - 18);
  bgSprite.setTextSize(2);
  bgSprite.setTextDatum(TR_DATUM);
  bgSprite.drawString(String(s_cpuLoad) + "%", barX + barW, barY_cpu - 18);
  // 進度條背景
  bgSprite.drawRoundRect(barX, barY_cpu, barW, barH, 2, C_GRID);
  // 進度條前景
  int cpuBarW = map(s_cpuLoad, 0, 100, 0, barW);
  if (cpuBarW > 0) {
    // 若負載超過 80% 則顯示紅色警示
    uint16_t barColor = (s_cpuLoad > 80) ? C_WARN : C_CPU;
    bgSprite.fillRoundRect(barX, barY_cpu, cpuBarW, barH, 2, barColor);
  }

  // RAM
  bgSprite.setTextColor(C_RAM, C_BG);
  bgSprite.setTextSize(2);
  bgSprite.setTextDatum(TL_DATUM);
  bgSprite.drawString("RAM", barX, barY_ram - 18);
  bgSprite.setTextSize(2);
  bgSprite.setTextDatum(TR_DATUM);
  bgSprite.drawString(String(s_ramLoad) + "%", barX + barW, barY_ram - 18);
  // 進度條背景
  bgSprite.drawRoundRect(barX, barY_ram, barW, barH, 2, C_GRID);
  // 進度條前景
  int ramBarW = map(s_ramLoad, 0, 100, 0, barW);
  if (ramBarW > 0) {
    uint16_t barColor = (s_ramLoad > 80) ? C_WARN : C_RAM;
    bgSprite.fillRoundRect(barX, barY_ram, ramBarW, barH, 2, barColor);
  }

  if (!isDataValid) drawDemoBox();
}

// --- 頁面 3: 詳細波形圖 ---
void drawBigGraphPage(bool isDataValid) {
  // 上半部: CPU
  drawBigGraphOnBg(cpuHistory, 0, SCREEN_H/2 - 1, C_CPU, "CPU", s_cpuLoad, "%");
  
  // 下半部: RAM
  drawBigGraphOnBg(ramHistory, SCREEN_H/2, SCREEN_H/2, C_RAM, "RAM", s_ramLoad, "%");

  // 中間分隔線
  bgSprite.drawFastHLine(0, SCREEN_H/2 - 1, SCREEN_W, C_GRID);

  if (!isDataValid) drawDemoBox();
}

// 繪製全寬度圖表 (直接畫在 bgSprite 上)
void drawBigGraphOnBg(float* history, int y, int h, uint16_t color, String label, int currentVal, String unit) {
  // 繪製背景格線
  bgSprite.drawFastHLine(0, y + h/2, SCREEN_W, C_GRID);
  
  float maxVal = 100.0;
  
  // 1. 繪製區域填充 (移植自 drawGraph 的漸層演算法)
  for (int i = 0; i < SCREEN_W; i++) {
    int valY = map((long)(history[i] * 10), 0, (long)(maxVal * 10), y + h - 1, y);
    if (valY < y) valY = y; if (valY >= y + h) valY = y + h - 1;

    int graphHeight = (y + h) - valY;

    for (int row = valY; row < y + h; row++) {
       // 背景是 C_BG，但中間有一條 C_GRID 格線
       uint16_t bg = (row == y + h/2) ? C_GRID : C_BG;

       // 計算透明度，創造由上而下變淡的效果
       int alpha = 0;
       if (graphHeight > 0) {
         alpha = 150 - (150 * (row - valY) / graphHeight);
       }
       
       if (alpha > 0) {
         uint16_t blended = tft.alphaBlend(alpha, color, bg);
         bgSprite.drawPixel(i, row, blended);
       }
    }
  }

  // 2. 繪製頂部線條 (保持銳利)
  for (int i = 0; i < SCREEN_W - 1; i++) {
    // 數據對應到高度
    int y1 = map((long)(history[i] * 10), 0, (long)(maxVal * 10), y + h - 1, y);
    int y2 = map((long)(history[i + 1] * 10), 0, (long)(maxVal * 10), y + h - 1, y);
    
    // 限制範圍
    if (y1 < y) y1 = y; if (y1 >= y + h) y1 = y + h - 1;
    if (y2 < y) y2 = y; if (y2 >= y + h) y2 = y + h - 1;

    bgSprite.drawLine(i, y1, i + 1, y2, color);
  }

  // 顯示標籤與數值 (左上角)
  bgSprite.setTextColor(C_TEXT); // 背景透明
  bgSprite.setTextSize(2);
  bgSprite.setTextDatum(TL_DATUM);
  bgSprite.drawString(label, 5, y + 5);

  // 數值 (右上角)
  bgSprite.setTextDatum(TR_DATUM);
  bgSprite.setTextColor(color); // 背景透明
  bgSprite.setTextSize(2);
  bgSprite.drawString(String(currentVal) + unit, SCREEN_W - 5, y + 5);
}

void drawDemoBox() {
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
  bgSprite.drawString("DEMO", SCREEN_W / 2, SCREEN_H / 2 + 3);
  bgSprite.setTextDatum(TL_DATUM); // 還原對齊設定
}

void updateHistory(float* history, float value) {
  for (int i = 0; i < HISTORY_LEN - 1; i++) {
    history[i] = history[i + 1];
  }
  history[HISTORY_LEN - 1] = value;
}

void drawGraph(TFT_eSprite &sprite, float* history, int x, int y, uint16_t color, float maxValOverride) {
  int w = sprite.width();
  int h = sprite.height();
  
  // 計算歷史資料的起始索引 (只取最後 w 筆)
  int startIdx = HISTORY_LEN - w;
  if (startIdx < 0) startIdx = 0;
  
  sprite.fillSprite(C_PANEL);
  sprite.drawFastHLine(0, 0, w, C_GRID);
  sprite.drawFastHLine(0, h/2, w, C_GRID);
  sprite.drawFastHLine(0, h-1, w, C_GRID);

  float maxVal = 1.0;
  if (maxValOverride > 0) {
    maxVal = maxValOverride;
  } else {
    for(int i=0; i<w; i++) if(history[startIdx + i] > maxVal) maxVal = history[startIdx + i];
  }

  // 1. 繪製區域填充 (Area Fill Gradient)
  for (int i = 0; i < w; i++) {
    int valY = map((long)(history[startIdx + i] * 10), 0, (long)(maxVal * 10), h - 1, 0);
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
  for (int i = 0; i < w - 1; i++) {
    int y1 = map((long)(history[startIdx + i] * 10), 0, (long)(maxVal * 10), h - 1, 0);
    int y2 = map((long)(history[startIdx + i + 1] * 10), 0, (long)(maxVal * 10), h - 1, 0);
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
