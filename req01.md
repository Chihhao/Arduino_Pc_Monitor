# 專案需求規格書 (Requirement Specification) - v1.0

## 1. 專案概述
本專案旨在將原有的 `Arduino_Pc_Monitor` 升級為現代化版本。
目標是利用 **LilyGO T-Display S3** 的彩色螢幕與高效能，取代原本的 Arduino Nano + LCD1602/2004 組合。
系統架構將從「PC端排版」轉型為「PC端傳送數據、裝置端繪製介面」的模式。

## 2. 系統架構
*   **PC 端 (Data Sender):** 
    *   負責採集硬體數據。
    *   不處理任何 UI 排版邏輯。
    *   將數據打包為 JSON 格式，透過 USB Serial 傳送。
*   **Device 端 (UI Renderer):** 
    *   接收 JSON 數據並解析。
    *   負責所有圖形繪製、顏色邏輯與歷史曲線運算。

## 3. 硬體規格
*   **開發板:** LilyGO T-Display S3 (ESP32-S3)
*   **螢幕:** 1.9吋 IPS LCD (解析度 170 x 320)
*   **連接介面:** USB Type-C (USB CDC Serial)
*   **顯示方向:** 橫向 (Landscape)

## 4. 軟體需求 (PC 端)
*   **開發環境:** .NET 8 (C#)
*   **類型:** Console Application (背景執行)
*   **核心函式庫:** `LibreHardwareMonitor`
*   **功能:**
    1.  每秒讀取一次系統資訊。
    2.  **CPU:** 讀取使用率 (Total Load)、封裝溫度 (Package Temp)。
    3.  **RAM:** 讀取使用率 (Load)、已用記憶體 (Used)、總記憶體 (Total)。
    4.  **Disk:** 讀取磁碟讀寫速度 (Read/Write)。
    5.  **Network:** 讀取所有網卡的總下載 (Download) 與上傳 (Upload) 速度。
    6.  **System:** 取得當前日期與時間。
    7.  將上述數據序列化為 JSON 字串並寫入 Serial Port。

## 5. 軟體需求 (Device 端)
*   **開發環境:** Arduino IDE / PlatformIO
*   **核心函式庫:** `TFT_eSPI` (繪圖), `ArduinoJson` (解析)
*   **介面佈局 (320x170):**
    *   **頂部狀態列 (Status Bar):** 分為 5 個區塊
        *   日期 (MM/DD)
        *   星期 (e.g., SUN)
        *   時間 (HH:MM)
        *   秒數 (SS)
        *   連線狀態指示燈
    *   **主畫面 (Main Dashboard):** 分為 4 個垂直欄位 (Columns)
        1.  **CPU:**
            *   上方: 溫度 (°C, >80°C 變紅)。
            *   下方: 使用率 (%, 大字體)。
            *   圖表: 歷史折線圖 (含漸層填充)。
        2.  **RAM:**
            *   上方: 已用容量 (GB)。
            *   下方: 使用率 (%, 大字體)。
            *   圖表: 歷史折線圖 (含漸層填充)。
        3.  **DISK:**
            *   上方: 讀取速度 (Read Speed)。
            *   下方: 寫入速度 (Write Speed)。
            *   圖表: 上下兩組獨立折線圖。
        4.  **NETWORK:**
            *   上方: 下載速度 (Download)。
            *   下方: 上傳速度 (Upload)。
            *   圖表: 上下兩組獨立折線圖。

## 6. 通訊協定 (Data Protocol)
*   **格式:** JSON
*   **頻率:** 1Hz (每秒一次)
*   **Baud Rate:** 921600
*   **JSON 範例:**
    ```json
    {
      "sys": {
        "date": "2023/10/27 (Fri)",
        "time": "14:30:05"
      },
      "cpu": {
        "load": 45.5,
        "temp": 55
      },
      "ram": {
        "load": 40.2,
        "used": 6.4,
        "total": 16.0
      },
      "disk": {
        "read": 10.5,
        "write": 5.2
      },
      "net": {
        "dl": 15.2,
        "ul": 2.1
      }
    }
    ```