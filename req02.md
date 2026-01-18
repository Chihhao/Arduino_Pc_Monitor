# 軟體需求規格書 (Software Requirement Specification)
**專案名稱：** Windows PC Hardware Monitor Client for T-Display S3
**版本：** 1.1 (WinForms Edition)
**日期：** 2026/01/18

## 1. 專案概述 (Project Overview)
本專案旨在開發一款 Windows 桌面應用程式，用於背景監控電腦硬體效能數據（CPU, RAM, Disk, Network），並透過 USB Serial 介面將數據即時傳送至外接的 **LilyGO T-Display S3** 顯示器。

本專案強調**現代化、精緻且整齊**的使用者介面設計，並要求在不依賴第三方 UI 套件的前提下，使用原生 WinForms 控制項實現高品質的視覺效果。

## 2. 技術規格 (Technical Stack)
*   **作業系統：** Windows 10 / 11 (64-bit)
*   **開發框架：** **.NET Framework 4.7.2**
*   **應用程式類型：** **Windows Forms (WinForms)**
*   **UI 限制：**
    *   **禁止使用第三方 UI 函式庫** (如 Guna UI, Bunifu, MetroFramework 等)。
    *   必須使用原生 `System.Windows.Forms` 控制項，透過屬性調整（如 `FlatStyle`, `BackColor`, `Padding`）與自繪（`OnPaint`）來達成現代化外觀。
*   **核心函式庫：**
    *   硬體監控：`LibreHardwareMonitorLib` (需透過 NuGet 安裝)
    *   JSON 處理：`Newtonsoft.Json` (建議) 或 `System.Web.Extensions`
    *   序列埠通訊：`System.IO.Ports`

## 3. 功能需求 (Functional Requirements)

### 3.1 硬體資訊採集 (Hardware Monitoring)
程式需每隔固定時間（預設 1000ms）讀取以下資訊：
1.  **System:** 日期 (YYYY/MM/DD)、時間 (HH:MM:SS)。
2.  **CPU:** 總使用率 (%)、封裝溫度 (°C)。
3.  **RAM:** 使用率 (%)、已用容量 (GB)、總容量 (GB)。
4.  **Disk:** 系統總讀取速度 (MB/s)、總寫入速度 (MB/s)。
5.  **Network:** 系統總下載速度 (MB/s)、總上傳速度 (MB/s)。

### 3.2 裝置自動偵測 (Auto-Detection)
程式啟動時需自動尋找 T-Display S3 裝置，無需使用者手動選擇 COM Port。
*   **握手流程：**
    1.  列舉所有 COM Ports。
    2.  以 BaudRate `115200` 開啟 Port。
    3.  發送字串 `"WHO\n"`。
    4.  若收到回應 `"TDISPLAYS3"` (去除空白後)，即鎖定該 Port 進行連線。

### 3.3 數據傳輸 (Data Transmission)
*   **格式：** JSON String + 換行符號 (`\n`)。
*   **內容結構：**
    ```json
    {
      "sys": { "date": "2026/01/18 (Sun)", "time": "14:30:05" },
      "cpu": { "load": 45.5, "temp": 55 },
      "ram": { "load": 40.2, "used": 6.4, "total": 16.0 },
      "disk": { "read": 10.5, "write": 5.2 },
      "net": { "dl": 15.2, "ul": 2.1 }
    }
    ```

## 4. UI 介面設計規範 (UI Design Guidelines)
**這是本專案的重點需求。** 雖然使用 WinForms，但**嚴禁**出現傳統 Windows 95/XP 風格的灰色立體按鈕或預設邊框。

### 4.1 整體風格 (Visual Style)
*   **主題色：** 深色模式 (Dark Mode)。
    *   背景色建議：`#1E1E1E` 或 `#252526` (VS Code 風格)。
    *   前景色/文字：`#FFFFFF` 或 `#E0E0E0`。
    *   強調色：`#007ACC` (藍色) 或 `#CE4000` (配合 Arduino 主題的暗黃色)。
*   **字體：** 使用系統無襯線字體 (如 `Segoe UI`)，大小需適中易讀 (9pt ~ 12pt)。
*   **扁平化設計 (Flat Design)：** 所有按鈕、輸入框皆不可有立體陰影或 3D 邊框。

### 4.2 視窗佈局 (Layout)
*   **無邊框視窗 (Borderless Window)：**
    *   設定 `FormBorderStyle = None`。
    *   需自行實作頂部的「標題列 (Title Bar)」，包含標題文字與關閉/最小化按鈕。
    *   視窗需支援滑鼠拖曳移動 (Drag to move)。
*   **對齊與間距：**
    *   嚴格使用 `TableLayoutPanel` 或 `FlowLayoutPanel` 進行排版，確保元件在視窗縮放或字體改變時保持整齊。
    *   元件之間需有足夠的 `Padding` 與 `Margin` (建議至少 10px)，避免擁擠。

### 4.3 控制項樣式細節 (Control Styling)
1.  **按鈕 (Button):**
    *   `FlatStyle` 設為 `Flat`。
    *   `FlatAppearance.BorderSize` 設為 `0` 或 `1`。
    *   滑鼠移入 (Hover) 與按下 (Click) 需有明顯的顏色變化回饋。
2.  **下拉選單 (ComboBox):**
    *   若原生 ComboBox 樣式與深色主題不合，需設定 `DrawMode = OwnerDrawFixed` 並自行繪製背景與文字顏色，或將 `FlatStyle` 設為 `Flat` 盡量融入背景。
3.  **狀態顯示區:**
    *   使用 `Label` 顯示數值，字體需加粗或放大以強調重點數據。
    *   可使用 `Panel` 作為容器，為不同區塊（如 CPU, RAM）繪製細微的背景色差或底線。

### 4.4 系統匣 (System Tray)
*   程式需包含 `NotifyIcon`。
*   點擊視窗「關閉」按鈕時，預設行為為「最小化至系統匣」而非結束程式。
*   右鍵選單需包含：顯示主視窗、重新連線、結束程式。

## 5. 交付項目 (Deliverables)
1.  **原始碼：** 完整的 Visual Studio Solution (.sln)，包含所有 .cs, .designer.cs, .resx 檔案。
2.  **執行檔：** 編譯後的 .exe 檔案 (Release Mode)。
3.  **說明文件：** 簡述如何設定開發環境 (如需安裝特定 NuGet 套件)。

## 6. 驗收標準 (Acceptance Criteria)
1.  **視覺驗收：** UI 必須看起來現代、乾淨。**若出現預設的灰色 WinForms 按鈕或不協調的配色，將視為不合格。**
2.  **功能驗收：** 插入 T-Display S3 後，程式能在 5 秒內自動連線並開始傳輸數據。
3.  **效能驗收：** 程式在背景執行時，CPU 佔用率應低於 1%。