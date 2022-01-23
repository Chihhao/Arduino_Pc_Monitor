using System;
using System.Windows.Forms;
using System.IO.Ports;
using System.Globalization;

using System.Collections.Generic;
using System.Diagnostics;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.ServiceProcess;
using System.Text;
using System.Text.RegularExpressions;
using System.Timers;

namespace Arduino_Monitor{
    public partial class Form1 : Form{

        const int LCD_BAR1 = 23;
        const int LCD_BAR2 = 24;
        const int LCD_BAR3 = 25;
        const int LCD_BAR4 = 26;
        const int LCD_BAR5 = 27;

        const int LCD_OPEN_BACK_LIGHT = 28;
        const int LCD_CLOSE_BACK_LIGHT = 29;
        const int LCD_CHANGE_LINE = 30;
        const int LCD_CLEAR = 31;

        const string HWiNFO_SHARED_MEM_FILE_NAME = "Global\\HWiNFO_SENS_SM2";
        const int HWiNFO_SENSORS_STRING_LEN = 128;
        const int HWiNFO_UNIT_STRING_LEN = 16;
        static MemoryMappedFile mmf;
        static MemoryMappedViewAccessor accessor;
        static HWiNFO_SHARED_MEM HWiNFOMemory;
        static List<HWiNFO_ELEMENT> data_arr; 

        string sOutputStr;

        private SerialPort port = new SerialPort();
        public Form1(){
            InitializeComponent();
            Init();
            btn_Connect_2.Enabled = true;
            btn_DisConnect.Enabled = false;
        }

        private void Init(){
            try
            {
                notifyIcon1.Visible = false;
                port.Parity = Parity.None;
                port.StopBits = StopBits.One;
                port.DataBits = 8;
                port.Handshake = Handshake.None;
                port.RtsEnable = true;
                string[] ports = SerialPort.GetPortNames();
                foreach (string p in ports)
                {
                    cb_COM.Items.Add(p);
                }
                cb_COM.Items.Add("TEST");
                port.BaudRate = 9600;

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void btn_DisConnect_Click(object sender, EventArgs e){ //離線按鈕
            toolStripStatusLabel1.Text = "Disconnected";
            tb_preview.Text = "";
            timer2.Enabled = false;
            try{
                if (port.IsOpen){
                    port.Write((char)(LCD_CLEAR) + "");
                    port.Write((char)(LCD_CLOSE_BACK_LIGHT) + "");
                    port.Close();
                }                
                btn_Connect_2.Enabled = true;
                btn_DisConnect.Enabled = false;
                cb_COM.Enabled = true;
            }
            catch (Exception ex){
                MessageBox.Show(ex.Message);
            }
        }

        
        private void btn_Connect2_Click(object sender, EventArgs e) { //連線 HWiNFO
            try{
                if (cb_COM.Text == "TEST"){
                    timer2.Interval = 1000;
                    timer2.Enabled = true;
                    toolStripStatusLabel1.Text = "Connected(TEST)";
                    tb_preview.Text = "";
                    btn_Connect_2.Enabled = false;
                    btn_DisConnect.Enabled = true;
                    cb_COM.Enabled = false;
                }
                else if (!port.IsOpen){
                    port.PortName = cb_COM.Text;
                    port.Open();
                    timer2.Interval = 1000;
                    timer2.Enabled = true;
                    timer1.Interval = 1000;
                    timer1.Enabled = true;
                    toolStripStatusLabel1.Text = "Connected";
                    tb_preview.Text = "";
                    port.Write((char)(LCD_OPEN_BACK_LIGHT) + "");
                    port.Write((char)(LCD_CLEAR) + "");
                    btn_Connect_2.Enabled = false;
                    btn_DisConnect.Enabled = true;
                    cb_COM.Enabled = false;
                }
            }
            catch (Exception ex){
                MessageBox.Show(ex.Message);
            }
        }

        private void Form1_Resize(object sender, EventArgs e){
            if (FormWindowState.Minimized == this.WindowState)
            {
                notifyIcon1.Visible = true;
                try
                {
                    notifyIcon1.ShowBalloonTip(500, "HWiNFO_LCD", toolStripStatusLabel1.Text, ToolTipIcon.Info);
                }
                catch (Exception ex)
                {

                }
                this.Hide();
            }
        }
        
        private void notifyIcon1_DoubleClick(object sender, EventArgs e){            
            this.Show();
            this.WindowState = FormWindowState.Normal;
            notifyIcon1.Visible = false;
        }
        
        private void Form1_Load_1(object sender, EventArgs e){
            toolStripStatusLabel1.Text = "Select Com Port and Click \"ON\"";
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HWiNFO_SHARED_MEM
        {
            public uint dwSignature;
            public uint dwVersion;
            public uint dwRevision;
            public long poll_time;
            public uint dwOffsetOfSensorSection;
            public uint dwSizeOfSensorElement;
            public uint dwNumSensorElements;
            public uint dwOffsetOfReadingSection;
            public uint dwSizeOfReadingElement;
            public uint dwNumReadingElements;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HWiNFO_ELEMENT
        {
            public SENSOR_READING_TYPE tReading;
            public uint dwSensorIndex;
            public uint dwReadingID;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = HWiNFO_SENSORS_STRING_LEN)]
            public string szLabelOrig;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = HWiNFO_SENSORS_STRING_LEN)]
            public string szLabelUser;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = HWiNFO_UNIT_STRING_LEN)]
            public string szUnit;
            public double Value;
            public double ValueMin;
            public double ValueMax;
            public double ValueAvg;
        }

        public enum SENSOR_READING_TYPE
        {
            SENSOR_TYPE_NONE,
            SENSOR_TYPE_TEMP,
            SENSOR_TYPE_VOLT,
            SENSOR_TYPE_FAN,
            SENSOR_TYPE_CURRENT,
            SENSOR_TYPE_POWER,
            SENSOR_TYPE_CLOCK,
            SENSOR_TYPE_USAGE,
            SENSOR_TYPE_OTHER,
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e){
            if (port.IsOpen){
                port.Write((char)(LCD_CLEAR) + "");
                port.Write((char)(LCD_CLOSE_BACK_LIGHT) + "");
                port.Close();
            } 
        }
        
        private void timer2_Tick(object sender, EventArgs e){
            if (!port.IsOpen && cb_COM.Text != "TEST"){
                port.PortName = cb_COM.Text;
                port.Open();
                return;
            }

            if (!HWiNFOisStarted()){
                toolStripStatusLabel1.Text = "HWiFNO App Error!";
                tb_preview.Text = "HWiFNO App Error!";
                if (port.IsOpen){
                    port.Write((char)(LCD_OPEN_BACK_LIGHT) + "");
                    port.Write((char)(LCD_CLEAR) + "");
                    port.Write("HWiFNO App Error!");
                    port.Write((char)(LCD_CHANGE_LINE) + "");
                }
                return;
            }
            
            if (!readMemory()){
                toolStripStatusLabel1.Text = "HWiFNO Sensor Error!";
                tb_preview.Text = "HWiFNO Sensor Error!";
                if (port.IsOpen){
                    port.Write((char)(LCD_OPEN_BACK_LIGHT) + "");
                    port.Write((char)(LCD_CLEAR) + "");
                    port.Write("HWiFNO Sensor Error!");
                    port.Write((char)(LCD_CHANGE_LINE) + "");
                }
                return;
            }

            if (data_arr.Count > 0){
                try{
                    //LIU();
                    HAO();
                }
                catch (Exception ex){
                    sendToPreviewScreen("serial error");
                    //eventLog.WriteEntry("Sending data to serial fail: " + ex.Message, EventLogEntryType.Error);
                    //port_opened = false;
                }
            }
        }

        private static bool HWiNFOisStarted(){
            Process[] processes = Process.GetProcesses();
            foreach (Process process in processes){
                if (Regex.IsMatch(process.ProcessName, "HWiNFO64")){
                    return true;
                }
            }
            return false;
        }

        private static bool readMemory(){
            try{
                mmf = MemoryMappedFile.OpenExisting(HWiNFO_SHARED_MEM_FILE_NAME, MemoryMappedFileRights.Read);
                accessor = mmf.CreateViewAccessor(0L, (long)Marshal.SizeOf(typeof(HWiNFO_SHARED_MEM)), MemoryMappedFileAccess.Read);
                accessor.Read<HWiNFO_SHARED_MEM>(0L, out HWiNFOMemory);
                data_arr = new List<HWiNFO_ELEMENT>();
                for (uint index = 0; index < HWiNFOMemory.dwNumReadingElements; ++index)
                {
                    using (MemoryMappedViewStream viewStream = mmf.CreateViewStream((long)(HWiNFOMemory.dwOffsetOfReadingSection + index * HWiNFOMemory.dwSizeOfReadingElement), (long)HWiNFOMemory.dwSizeOfReadingElement, MemoryMappedFileAccess.Read))
                    {
                        byte[] buffer = new byte[new IntPtr(HWiNFOMemory.dwSizeOfReadingElement).ToInt32()];
                        viewStream.Read(buffer, 0, (int)HWiNFOMemory.dwSizeOfReadingElement);
                        GCHandle gcHandle = GCHandle.Alloc((object)buffer, GCHandleType.Pinned);
                        HWiNFO_ELEMENT structure = (HWiNFO_ELEMENT)Marshal.PtrToStructure(gcHandle.AddrOfPinnedObject(), typeof(HWiNFO_ELEMENT));
                        gcHandle.Free();
                        data_arr.Add(structure);
                    }
                }
                return true;
            }
            catch (Exception ex){
                //eventLog.WriteEntry("An error occured while opening the HWiNFO shared memory: " + ex.Message, EventLogEntryType.Error);
                return false;
            }
        }

        int iPreviewLine = 1;
        string sPreviewStr1 = "";
        string sPreviewStr2 = "";
        string sPreviewStr3 = "";
        string sPreviewStr4 = "";
        private void sendToPreviewScreen(string inStr){
            inStr = inStr.Replace((char)LCD_BAR1, '2');
            inStr = inStr.Replace((char)LCD_BAR2, '4');
            inStr = inStr.Replace((char)LCD_BAR3, '6');
            inStr = inStr.Replace((char)LCD_BAR4, '8');
            inStr = inStr.Replace((char)LCD_BAR5, '=');

            if (iPreviewLine == 1) sPreviewStr1 = inStr;
            if (iPreviewLine == 2) sPreviewStr2 = inStr;
            if (iPreviewLine == 3) sPreviewStr3 = inStr;
            if (iPreviewLine == 4) sPreviewStr4 = inStr;
            iPreviewLine += 1;
            if (iPreviewLine > 4) iPreviewLine = 1;
            tb_preview.Text = sPreviewStr1 + "\n" + sPreviewStr2 + "\n" +
                              sPreviewStr3 + "\n" + sPreviewStr4;
        }

        private void LIU(){
            float fTmp;
            int iTmp1, iTmp2, iTmp3, iTmp4, iTmp5;

            //LINE1
            //Cpu|0000@1.000V|55'c
            //Cpu| Clocks@Voltage(SVI2 TFN)|CPU(Tctl/Tdie)
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "Core 0 Clock (perf #1/4)").Value);
            fTmp = (float)(data_arr.Find(d => d.szLabelOrig == "CPU Core Voltage (SVI2 TFN)").Value);
            iTmp2 = (int)(data_arr.Find(d => d.szLabelOrig == "CPU (Tctl/Tdie)").Value);
            sOutputStr = String.Format("Cpu|{0,4}@{1:0.000}V|{2,2}'c", iTmp1, fTmp, iTmp2);
            sendToPreviewScreen(sOutputStr);
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }

            //LINE2
            //Ram|3200@14-14-14-28
            //Ram| Memory Clock*2@Tcas-Trcd-Trp-Tras
            /*
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "Memory Clock").Value)*2;                    
            iTmp2 = (int)(data_arr.Find(d => d.szLabelOrig == "Tcas").Value);
            iTmp3 = (int)(data_arr.Find(d => d.szLabelOrig == "Trcd").Value);
            iTmp4 = (int)(data_arr.Find(d => d.szLabelOrig == "Trp").Value);
            iTmp5 = (int)(data_arr.Find(d => d.szLabelOrig == "Tras").Value);
            sOutputStr = String.Format("Ram|{0,4}@{1,2}-{2,2}-{3,2}-{4,2}", iTmp1, iTmp2, iTmp3, iTmp4, iTmp5);
            */

            //Ram|3200@1.000V|55'c
            //Ram|Memory Clock*2@DRAM|DIMM[2] Temperature+DIMM[3] Temperature平均
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "Memory Clock").Value) * 2;
            fTmp = (float)(data_arr.Find(d => d.szLabelOrig == "DRAM").Value);
            iTmp2 = (int)(data_arr.Find(d => d.szLabelOrig == "DIMM[2] Temperature").Value);
            iTmp3 = (int)(data_arr.Find(d => d.szLabelOrig == "DIMM[3] Temperature").Value);
            iTmp4 = (iTmp2 + iTmp3) / 2;
            sOutputStr = String.Format("Ram|{0,4}@{1:0.000}V|{2,2}'c", iTmp1, fTmp, iTmp4);
            sendToPreviewScreen(sOutputStr);
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }

            //LINE3
            //Gpu|0000/0000  |35'c
            //Gpu| GPU Core Clock/GPU Memory Clock  |GPU Temp
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "GPU Clock").Value);
            iTmp2 = (int)(data_arr.Find(d => d.szLabelOrig == "GPU Memory Clock").Value);
            iTmp3 = (int)(data_arr.Find(d => d.szLabelOrig == "GPU Temperature").Value);
            sOutputStr = String.Format("Gpu|{0,4}/{1,4}  |{2,2}'c", iTmp1, iTmp2, iTmp3);
            sendToPreviewScreen(sOutputStr);
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }

            //LINE4
            //Fan|0000   PUMP|0000
            //Fan| CPU1   PUMP|CPU2
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "CPU1" && d.szUnit == "RPM").Value);
            iTmp2 = (int)(data_arr.Find(d => d.szLabelOrig == "CPU2" && d.szUnit == "RPM").Value);
            sOutputStr = String.Format("Fan|{0,4}   Pump|{1,4}", iTmp1, iTmp2);
            sendToPreviewScreen(sOutputStr);
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }
        }

        static bool bColon = true; //閃爍的冒號
        private void HAO(){
            float fTmp;
            int iTmp1, iTmp2, iTmp3, iTmp4, iTmp5;
            int strlen=0;
            

            //LINE1
            DateTime now = DateTime.Now;
            if (bColon == true) {
                sOutputStr = now.ToString("ddd yyyy-MM-dd HH:mm", new CultureInfo("en-US"));
                bColon = false;
            }
            else {
                sOutputStr = now.ToString("ddd yyyy-MM-dd HH mm", new CultureInfo("en-US"));
                bColon = true;
            }
            
            sendToPreviewScreen(sOutputStr.ToUpper());
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }

            //LINE2
            sOutputStr = "Cpu ";
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "Total CPU Usage").Value);
            iTmp2 = (int)(data_arr.Find(d => d.szLabelOrig == "CPU Package").Value);
            if (iTmp1 > 99) iTmp1 = 99;
            for (int i = 0; i < iTmp1 / 10; i++) sOutputStr += (char)(LCD_BAR5);
            if (iTmp1 % 10 >= 9) sOutputStr += (char)(LCD_BAR5);
            else if (iTmp1 % 10 >= 7) sOutputStr += (char)(LCD_BAR4);
            else if (iTmp1 % 10 >= 5) sOutputStr += (char)(LCD_BAR3);
            else if (iTmp1 % 10 >= 3) sOutputStr += (char)(LCD_BAR2);
            else if (iTmp1 % 10 >= 1) sOutputStr += (char)(LCD_BAR1);               
            strlen=sOutputStr.Length;
            for (int i = 0; i < 14 - strlen; i++) sOutputStr += " ";
            sOutputStr += String.Format("| {0,2}'c", iTmp2);
            sendToPreviewScreen(sOutputStr);
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }

            //LINE3
            sOutputStr = "Ram ";
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "Physical Memory Load").Value);
            fTmp = (float)(data_arr.Find(d => d.szLabelOrig == "Physical Memory Used").Value);
            if (iTmp1 > 99) iTmp1 = 99;
            for (int i = 0; i < iTmp1 / 10; i++) sOutputStr += (char)(LCD_BAR5);
            if (iTmp1 % 10 >= 9) sOutputStr += (char)(LCD_BAR5);
            else if (iTmp1 % 10 >= 7) sOutputStr += (char)(LCD_BAR4);
            else if (iTmp1 % 10 >= 5) sOutputStr += (char)(LCD_BAR3);
            else if (iTmp1 % 10 >= 3) sOutputStr += (char)(LCD_BAR2);
            else if (iTmp1 % 10 >= 1) sOutputStr += (char)(LCD_BAR1);
            strlen = sOutputStr.Length;
            for (int i = 0; i < 14 - strlen; i++) sOutputStr += " ";
            sOutputStr += String.Format("|{0,4:0.0}G", fTmp/1024);
            sendToPreviewScreen(sOutputStr);
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }

            //LINE4
            sOutputStr = "Gpu ";
            iTmp1 = (int)(data_arr.Find(d => d.szLabelOrig == "GPU Core Load").Value);
            iTmp2 = (int)(data_arr.Find(d => d.szLabelOrig == "GPU Temperature").Value);
            if (iTmp1 > 99) iTmp1 = 99;
            for (int i = 0; i < iTmp1 / 10; i++) sOutputStr += (char)(LCD_BAR5);
            if (iTmp1 % 10 >= 9) sOutputStr += (char)(LCD_BAR5);
            else if (iTmp1 % 10 >= 7) sOutputStr += (char)(LCD_BAR4);
            else if (iTmp1 % 10 >= 5) sOutputStr += (char)(LCD_BAR3);
            else if (iTmp1 % 10 >= 3) sOutputStr += (char)(LCD_BAR2);
            else if (iTmp1 % 10 >= 1) sOutputStr += (char)(LCD_BAR1);
            strlen = sOutputStr.Length;
            for (int i = 0; i < 14 - strlen; i++) sOutputStr += " ";
            sOutputStr += String.Format("| {0,2}'c", iTmp2);
            sendToPreviewScreen(sOutputStr);
            if (port.IsOpen) { port.Write(sOutputStr + (char)(LCD_CHANGE_LINE)); }
        
        }

        private void timer1_Tick(object sender, EventArgs e){
            if (cb_COM.Text == "") { return; }
            if (checkBox1.Checked == false) { return; }

            DateTime now = DateTime.Now;
            if (now.DayOfWeek == DayOfWeek.Sunday || now.DayOfWeek == DayOfWeek.Saturday) {
                if (btn_Connect_2.Enabled == true) { 
                    btn_Connect2_Click(sender, e); 
                }
            }
            else{
                if (now.Hour >= 0 && now.Hour < 18){ //不使用電腦的時間
                    if (btn_DisConnect.Enabled == true){
                        btn_DisConnect_Click(sender, e);                    
                    }
                }
                else{
                    if (btn_Connect_2.Enabled == true){
                        btn_Connect2_Click(sender, e);
                    }
                }         
            }
        }
        
    }
}
