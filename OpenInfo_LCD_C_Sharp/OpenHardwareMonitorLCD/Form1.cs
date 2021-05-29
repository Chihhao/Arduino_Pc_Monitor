using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using OpenHardwareMonitor.Hardware;

namespace OpenHardwareMonitorLCD{
    public partial class Form1 : Form{
        const string VERSION = "2021-05-17";
        const int LCD_BAR1 = 23;
        const int LCD_BAR2 = 24;
        const int LCD_BAR3 = 25;
        const int LCD_BAR4 = 26;
        const int LCD_BAR5 = 27;
        const int LCD_OPEN_BACK_LIGHT = 28;
        const int LCD_CLOSE_BACK_LIGHT = 29;
        const int LCD_CHANGE_LINE = 30;
        const int LCD_CLEAR = 31;
        
        Computer c = new Computer()
        {
            CPUEnabled = true, 
            RAMEnabled = true
        };
        float fCpuUsage, fMemUsage;
        string sOutputStr;
        bool bLCD_Enable = false;

        private SerialPort port = new SerialPort();
        public Form1(){
            InitializeComponent();
            Init();
        }

        private void Init()
        {
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
                    comboBox1.Items.Add(p);
                }
                port.BaudRate = 9600;
                timer1.Interval = 1000;
                timer1.Enabled = true; 

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            try{
                if (port.IsOpen){
                    port.Write((char)(LCD_CLEAR) + "");
                    port.Write((char)(LCD_CLOSE_BACK_LIGHT) + "");
                    port.Close();
                }
                button1.Enabled = true;
                button2.Enabled = false;
                comboBox1.Enabled = true;
                bLCD_Enable = false;
            }
            catch (Exception ex){
                MessageBox.Show(ex.Message);
            }
            toolStripStatusLabel1.Text = "Disconnected";
            //timer1.Enabled = false; 

        }

        private void button1_Click(object sender, EventArgs e)
        {
            try
            {
                if (!port.IsOpen){                    
                    port.PortName = comboBox1.Text;
                    port.Open();

                    //timer1.Interval = 1000;
                    //timer1.Enabled = true;
                    toolStripStatusLabel1.Text = port.PortName + " Connected";
                    notifyIcon1.Text = port.PortName + " Connected";
                    port.Write((char)(LCD_OPEN_BACK_LIGHT) + "");
                    port.Write((char)(LCD_CLEAR) + "");
                }
                button1.Enabled = false;
                button2.Enabled = true;
                comboBox1.Enabled = false;
                bLCD_Enable = true;

            }
            catch (Exception ex){
                MessageBox.Show(ex.Message);
            }
        }

        private void timer1_Tick(object sender, EventArgs e){
            /*
            DateTime tNow = DateTime.Now;            
            DateTime tBreak = DateTime.Parse("17:29:59");
            DateTime tWork = DateTime.Parse("08:29:59");
            if (button2.Enabled == true && tNow.Hour == tBreak.Hour && tNow.Minute == tBreak.Minute && tNow.Second == tBreak.Second) {
                button2_Click(sender, e);
            }
            if (button1.Enabled == true && tNow.Hour == tWork.Hour && tNow.Minute == tWork.Minute && tNow.Second == tWork.Second) {
                button1_Click(sender, e);
            }
            */
            if (bLCD_Enable) {
                Status();
            }            
        }

        private void Form1_Resize(object sender, EventArgs e){
            if (FormWindowState.Minimized == this.WindowState)
            {
                notifyIcon1.Visible = true;
                try
                {
                    notifyIcon1.ShowBalloonTip(500, "OpenHardwareMonitorLCD", toolStripStatusLabel1.Text, ToolTipIcon.Info);

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
        
        private void Status(){
            foreach (var hardwadre in c.Hardware){
                if (hardwadre.HardwareType == HardwareType.CPU){
                    hardwadre.Update();
                    foreach (var sensor in hardwadre.Sensors){                        
                        if (sensor.Name == "CPU Total" && sensor.SensorType == SensorType.Load){
                            fCpuUsage = sensor.Value.GetValueOrDefault();
                            //if (fCpuUsage >= 99.0) fCpuUsage = 99;
                        }
                    }
                }
                else if (hardwadre.HardwareType == HardwareType.RAM){
                    hardwadre.Update();
                    foreach (var sensor in hardwadre.Sensors){
                        if (sensor.Name == "Memory" && sensor.SensorType == SensorType.Load){
                            fMemUsage = sensor.Value.GetValueOrDefault();
                            //if (fMemUsage >= 99.0) fMemUsage = 99;
                        }
                    }
                }
            }

            try{
                DateTime tNow = DateTime.Now;
                sOutputStr = "";
                sOutputStr += "C";
                if (fCpuUsage >= 33.30) sOutputStr += (char)LCD_BAR5;
                if (fCpuUsage >= 66.60) sOutputStr += (char)LCD_BAR5;                
                if ((fCpuUsage % 33.30) <= 6.66) sOutputStr += (char)LCD_BAR1;
                else if ((fCpuUsage % 33.30) <= 13.32) sOutputStr += (char)LCD_BAR2;
                else if ((fCpuUsage % 33.30) <= 19.98) sOutputStr += (char)LCD_BAR3;
                else if ((fCpuUsage % 33.30) <= 26.64) sOutputStr += (char)LCD_BAR4;
                else if ((fCpuUsage % 33.30) <= 33.30) sOutputStr += (char)LCD_BAR5;
                if (fCpuUsage < 66.60) sOutputStr += " ";
                if (fCpuUsage < 33.30) sOutputStr += " ";
                if (fCpuUsage < 10) { sOutputStr += " "; }
                if (fCpuUsage > 99) { fCpuUsage = 99; }

                sOutputStr += (int)fCpuUsage + "%";
                sOutputStr += " ";
                sOutputStr += tNow.Month.ToString("d2") + "-" + tNow.Day.ToString("d2");
                sOutputStr += "(" + (int)tNow.DayOfWeek + ")";
                port.Write(sOutputStr);
                port.Write((char)LCD_CHANGE_LINE + ""); //new line on screen

                sOutputStr = "";
                sOutputStr += "R";
                if (fMemUsage >= 33.30) sOutputStr += (char)LCD_BAR5;
                if (fMemUsage >= 66.60) sOutputStr += (char)LCD_BAR5;
                if ((fMemUsage % 33.30) <= 6.66) sOutputStr += (char)LCD_BAR1;
                else if ((fMemUsage % 33.30) <= 13.32) sOutputStr += (char)LCD_BAR2;
                else if ((fMemUsage % 33.30) <= 19.98) sOutputStr += (char)LCD_BAR3;
                else if ((fMemUsage % 33.30) <= 26.64) sOutputStr += (char)LCD_BAR4;
                else if ((fMemUsage % 33.30) <= 33.30) sOutputStr += (char)LCD_BAR5;
                if (fMemUsage < 66.60) sOutputStr += " ";
                if (fMemUsage < 33.30) sOutputStr += " ";
                if (fMemUsage < 10) { sOutputStr += " "; }
                if (fMemUsage > 99) { fMemUsage = 99; }

                sOutputStr += (int)fMemUsage + "%";
                sOutputStr += " ";
                sOutputStr += tNow.Hour.ToString("d2") + ":" + tNow.Minute.ToString("d2");
                sOutputStr += ":" + tNow.Second.ToString("d2");
                port.Write(sOutputStr);
                port.Write((char)LCD_CHANGE_LINE + ""); //new line on screen                
                               
            }catch(Exception ex){
                timer1.Stop();
                MessageBox.Show(ex.Message);
                toolStripStatusLabel1.Text = "not responding...";
            }
        }

        private void Form1_Load_1(object sender, EventArgs e){
            c.Open();
            toolStripStatusLabel1.Text = "Select Com Port and click \"ON\"";
            this.Text = "CPU Monitor " + VERSION;
        }

     }
}
