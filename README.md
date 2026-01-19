# Arduino_Pc_Monitor (電腦資源監視器)

這是一個基於 Arduino 與 C# 的電腦資源監控專案，透過外接 LCD 螢幕即時顯示 PC 的硬體狀態。
系統採用 Master-Slave 架構，必須在 PC 端執行一個常駐程式以傳送數據。

## 版本說明
本專案提供兩個版本，分別針對不同的需求與硬體配置：

### 1. OpenInfo_LCD (使用 LCD1602)
   (1) OpenInfo_LCD_C#: C# 原始碼 (.net 4.0)  
   (2) OpenHardwareMonitorLCD.exe: 已Compile的執行檔  
   (3) OpenHardwareMonitorLib.dll: 必須與執行檔放在相同目錄  
   (4) OpenInfo_LCD_Arduino: Arduino 原始碼  
   (5) 硬體材料: Arduino Nano, LCD1602(I2C)  
   (6) 3D列印外殼: https://www.thingiverse.com/thing:3350530  
  
### 2. HWiNFO_LCD (使用 LCD2004)
   (1) HWiNFO_LCD_C#: C# 原始碼 (.net 4.0)  
   (2) HWiNFO_LCD.exe: 已Compile的執行檔  
   (3) 必須安裝 HWiNFO，官方網站: https://www.hwinfo.com/  
   (4) 在 HWiNFO Settings 中，勾選 Shared Memory Support  
   (5) 在 HWiNFO Sensor Settings 中，勾選 Enable reporting Gadget  
   (6) HWiNFO_LCD_Arduino: Arduino 原始碼  
   (7) 硬體材料: Arduino Nano, LCD2004(I2C)  
   (8) 3D列印外殼: https://www.thingiverse.com/thing:4171171  

### 3. T-Display S3 Monitor (使用 LilyGO T-Display S3). 
   (1) PC Client: C# WinForms (.NET Framework 4.7.2)
   (2) Firmware: Arduino (ESP32-S3)  
   (3) 硬體材料: LilyGO T-Display S3 (1.9吋 IPS 彩色螢幕)  
   (4) 特色: USB Type-C 連接, 高速傳輸 (921600 baud), 支援 CPU/RAM/Disk/Network 監控與歷史曲線  

## 成果展示
[![Watch the video](https://raw.githubusercontent.com/gabriel-vasile/mimetype-icon-js/master/images/play-button.png)](https://www.youtube.com/watch?v=AJtOAfcR8xE)
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/wiring.png)
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/1.jpg)
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/2.jpg)
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/3.png)
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/4.jpg)
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/5.png)  
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/HWiNFO64_Settings_1.png)  
![image](https://github.com/Chihhao/Arduino_Pc_Monitor/blob/master/image/HWiNFO64_Settings_2.png)
