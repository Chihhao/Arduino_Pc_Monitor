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
    2.  **CPU:** 讀取使用率 (Total Load)、封裝溫度 (Package Temp)、封裝功耗 (Package Power)。
    3.  **RAM:** 讀取使用率 (Load)、已用記憶體 (Used)、總記憶體 (Total)。
    4.  **Network:** 讀取所有網卡的總下載 (Download) 與上傳 (Upload) 速度。
    5.  **System:** 取得當前日期與時間。
    6.  將上述數據序列化為 JSON 字串並寫入 Serial Port。

## 5. 軟體需求 (Device 端)
*   **開發環境:** Arduino IDE / PlatformIO
*   **核心函式庫:** `TFT_eSPI` (繪圖), `ArduinoJson` (解析)
*   **介面佈局 (320x170):**
    *   **頂部狀態列 (Status Bar):** 
        *   左側: 日期 (e.g., "Oct 27")
        *   右側: 時間 (e.g., "14:30:05")
    *   **左側主區塊 (CPU):** 
        *   顯示 CPU Load (大字體)。
        *   顯示溫度 (動態變色: >80°C 紅色, 否則白色)。
        *   顯示功耗 (Watts)。
        *   **圖表:** 繪製 CPU Load 或 Temp 的歷史折線圖 (Line Chart)。
    *   **右上方區塊 (RAM):**
        *   顯示 RAM Load %。
        *   顯示 "Used / Total GB"。
        *   **圖表:** 簡單的長條進度條 (Progress Bar)。
    *   **右下方區塊 (Network):**
        *   顯示下載速度 (e.g., "DL: 15.2 MB/s")。
        *   顯示上傳速度 (e.g., "UL: 2.1 MB/s")。

## 6. 通訊協定 (Data Protocol)
*   **格式:** JSON
*   **頻率:** 1Hz (每秒一次)
*   **Baud Rate:** 115200 (或更高，視 USB CDC 效能而定)
*   **JSON 範例:**
    ```json
    {
      "sys": {
        "date": "2023-10-27",
        "time": "14:30:05"
      },
      "cpu": {
        "name": "i7-12700",
        "load": 45.5,
        "temp": 55,
        "watt": 65.5
      },
      "ram": {
        "load": 40.2,
        "used": 6.4,
        "total": 16.0
      },
      "net": {
        "dl": 15.2,
        "ul": 2.1
      }
    }
    ```