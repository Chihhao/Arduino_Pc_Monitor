import time
import json
import serial
import psutil
import platform
from datetime import datetime

# ================= 設定區域 =================
# 請修改為您的 ESP32 埠號 (在終端機輸入 ls /dev/tty.* 查看)
SERIAL_PORT = '/dev/tty.usbmodem1101' 
BAUD_RATE = 115200
# ===========================================

def get_cpu_temp():
    """
    獲取 CPU 溫度。
    注意：macOS (特別是 Apple Silicon) 的溫度感測器無法透過標準 psutil 直接讀取。
    通常需要 root 權限執行 powermetrics 指令才能獲取。
    為了測試腳本的簡單性，這裡回傳一個模擬值 (40~50度)。
    """
    # 如果您有安裝特定的溫度讀取工具，可以在此修改
    return 45

def main():
    print(f"--- Mac 系統監控傳輸工具 ---")
    print(f"目標埠號: {SERIAL_PORT}")
    print(f"傳輸速率: {BAUD_RATE}")
    print("正在連線...")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("✅ 連線成功！數據傳輸中... (按 Ctrl+C 停止)")
    except serial.SerialException as e:
        print(f"\n❌ 無法開啟序列埠：{e}")
        print("請檢查：\n1. ESP32 是否已連接\n2. 埠號是否正確 (ls /dev/tty.*)\n3. 是否有其他程式佔用了該埠口")
        return

    # 初始化計數器 (用於計算網速與硬碟讀寫速度)
    last_net = psutil.net_io_counters()
    last_disk = psutil.disk_io_counters()
    last_time = time.time()
    
    # 讓 psutil CPU 第一次讀取歸零 (第一次呼叫通常會回傳 0)
    psutil.cpu_percent(interval=None)

    try:
        while True:
            # --- 1. 控制刷新率與 CPU 計算 ---
            # interval=0.1 會讓程式暫停 0.1 秒，並計算這段時間內的 CPU 平均負載
            # 這剛好讓我們達到約 10 FPS 的傳輸頻率
            cpu_load = psutil.cpu_percent(interval=0.1)
            
            current_time = time.time()
            time_delta = current_time - last_time
            
            # 避免時間差過小導致除以零
            if time_delta <= 0:
                time_delta = 0.1

            # --- 2. 準備數據 ---
            
            # 時間
            now = datetime.now()
            # 格式化為 Arduino 預期的格式: "YYYY/MM/DD (Day)"
            date_str = f"{now.strftime('%Y/%m/%d')} ({now.strftime('%a')})"
            time_str = now.strftime("%H:%M:%S")

            # RAM (GB)
            mem = psutil.virtual_memory()
            ram_load = mem.percent
            ram_used = mem.used / (1024 ** 3) 
            ram_total = mem.total / (1024 ** 3)

            # Disk I/O (換算為 MB/s)
            disk = psutil.disk_io_counters()
            disk_r_mb = (disk.read_bytes - last_disk.read_bytes) / time_delta / (1024 * 1024)
            disk_w_mb = (disk.write_bytes - last_disk.write_bytes) / time_delta / (1024 * 1024)
            
            # Network I/O (換算為 MB/s)
            net = psutil.net_io_counters()
            net_dl_mb = (net.bytes_recv - last_net.bytes_recv) / time_delta / (1024 * 1024)
            net_ul_mb = (net.bytes_sent - last_net.bytes_sent) / time_delta / (1024 * 1024)

            # 更新上一幀的狀態
            last_disk = disk
            last_net = net
            last_time = current_time

            # --- 3. 組合 JSON ---
            # 結構必須與 Arduino 的 parseAndDisplay 對應
            data = {
                "sys": {
                    "date": date_str,
                    "time": time_str
                },
                "cpu": {
                    "load": round(cpu_load, 1),
                    "temp": get_cpu_temp()
                },
                "ram": {
                    "load": round(ram_load, 1),
                    "used": round(ram_used, 1),
                    "total": round(ram_total, 1)
                },
                "disk": {
                    "read": round(disk_r_mb, 1),
                    "write": round(disk_w_mb, 1)
                },
                "net": {
                    "dl": round(net_dl_mb, 1),
                    "ul": round(net_ul_mb, 1)
                }
            }

            # --- 4. 發送數據 ---
            json_output = json.dumps(data)
            # 加上換行符號 \n 是關鍵，因為 Arduino 使用 readStringUntil('\n')
            ser.write((json_output + '\n').encode('utf-8'))
            
            # 在終端機印出簡單狀態以確認運作中
            # print(f"Sent: CPU {cpu_load}% | RAM {ram_load}%")

    except KeyboardInterrupt:
        print("\n程式已停止")
        ser.close()
    except Exception as e:
        print(f"發生錯誤：{e}")
        if ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()
