namespace Arduino_Monitor
{
    partial class Form1
    {
        /// <summary>
        /// 設計工具所需的變數。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清除任何使用中的資源。
        /// </summary>
        /// <param name="disposing">如果應該處置 Managed 資源則為 true，否則為 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form 設計工具產生的程式碼

        /// <summary>
        /// 此為設計工具支援所需的方法 - 請勿使用程式碼編輯器
        /// 修改這個方法的內容。
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.btn_DisConnect = new System.Windows.Forms.Button();
            this.notifyIcon1 = new System.Windows.Forms.NotifyIcon(this.components);
            this.cb_COM = new System.Windows.Forms.ComboBox();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.btn_Connect_2 = new System.Windows.Forms.Button();
            this.timer2 = new System.Windows.Forms.Timer(this.components);
            this.tb_preview = new System.Windows.Forms.TextBox();
            this.label_ver = new System.Windows.Forms.Label();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.statusStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btn_DisConnect
            // 
            this.btn_DisConnect.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_DisConnect.Font = new System.Drawing.Font("微軟正黑體", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(136)));
            this.btn_DisConnect.Location = new System.Drawing.Point(362, 18);
            this.btn_DisConnect.Margin = new System.Windows.Forms.Padding(2);
            this.btn_DisConnect.Name = "btn_DisConnect";
            this.btn_DisConnect.Size = new System.Drawing.Size(87, 40);
            this.btn_DisConnect.TabIndex = 2;
            this.btn_DisConnect.Text = "OFF";
            this.btn_DisConnect.UseVisualStyleBackColor = true;
            this.btn_DisConnect.Click += new System.EventHandler(this.Btn_DisConnect_Click);
            // 
            // notifyIcon1
            // 
            this.notifyIcon1.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon1.Icon")));
            this.notifyIcon1.Text = "HWiNFO_LCD";
            this.notifyIcon1.Visible = true;
            this.notifyIcon1.DoubleClick += new System.EventHandler(this.NotifyIcon1_DoubleClick);
            // 
            // cb_COM
            // 
            this.cb_COM.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cb_COM.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.cb_COM.Font = new System.Drawing.Font("微軟正黑體", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(136)));
            this.cb_COM.FormattingEnabled = true;
            this.cb_COM.Location = new System.Drawing.Point(18, 18);
            this.cb_COM.Margin = new System.Windows.Forms.Padding(4);
            this.cb_COM.Name = "cb_COM";
            this.cb_COM.Size = new System.Drawing.Size(132, 38);
            this.cb_COM.Sorted = true;
            this.cb_COM.TabIndex = 0;
            // 
            // statusStrip1
            // 
            this.statusStrip1.ImageScalingSize = new System.Drawing.Size(24, 24);
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1});
            this.statusStrip1.Location = new System.Drawing.Point(1, 286);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Padding = new System.Windows.Forms.Padding(2, 0, 21, 0);
            this.statusStrip1.Size = new System.Drawing.Size(464, 30);
            this.statusStrip1.SizingGrip = false;
            this.statusStrip1.TabIndex = 6;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(192, 23);
            this.toolStripStatusLabel1.Text = "toolStripStatusLabel1";
            // 
            // btn_Connect_2
            // 
            this.btn_Connect_2.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_Connect_2.Font = new System.Drawing.Font("微軟正黑體", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(136)));
            this.btn_Connect_2.Location = new System.Drawing.Point(158, 18);
            this.btn_Connect_2.Margin = new System.Windows.Forms.Padding(4);
            this.btn_Connect_2.Name = "btn_Connect_2";
            this.btn_Connect_2.Size = new System.Drawing.Size(194, 40);
            this.btn_Connect_2.TabIndex = 1;
            this.btn_Connect_2.Text = "ON (HWiNFO)";
            this.btn_Connect_2.UseVisualStyleBackColor = true;
            this.btn_Connect_2.Click += new System.EventHandler(this.Btn_Connect2_Click);
            // 
            // timer2
            // 
            this.timer2.Tick += new System.EventHandler(this.Timer2_Tick);
            // 
            // tb_preview
            // 
            this.tb_preview.BackColor = System.Drawing.Color.Black;
            this.tb_preview.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tb_preview.Font = new System.Drawing.Font("Consolas", 18F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tb_preview.ForeColor = System.Drawing.Color.White;
            this.tb_preview.Location = new System.Drawing.Point(18, 70);
            this.tb_preview.Multiline = true;
            this.tb_preview.Name = "tb_preview";
            this.tb_preview.ReadOnly = true;
            this.tb_preview.Size = new System.Drawing.Size(431, 173);
            this.tb_preview.TabIndex = 7;
            this.tb_preview.TabStop = false;
            this.tb_preview.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // label_ver
            // 
            this.label_ver.Font = new System.Drawing.Font("微軟正黑體", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(136)));
            this.label_ver.ForeColor = System.Drawing.Color.Gray;
            this.label_ver.Location = new System.Drawing.Point(217, 243);
            this.label_ver.Name = "label_ver";
            this.label_ver.Size = new System.Drawing.Size(232, 30);
            this.label_ver.TabIndex = 8;
            this.label_ver.Text = "ver: xxxx/xx/xx";
            this.label_ver.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // timer1
            // 
            this.timer1.Interval = 1000;
            this.timer1.Tick += new System.EventHandler(this.Timer1_Tick);
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Font = new System.Drawing.Font("微軟正黑體", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(136)));
            this.checkBox1.Location = new System.Drawing.Point(18, 248);
            this.checkBox1.Margin = new System.Windows.Forms.Padding(2);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(182, 27);
            this.checkBox1.TabIndex = 9;
            this.checkBox1.Text = "Only 18:00~24:00";
            this.checkBox1.UseVisualStyleBackColor = true;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 18F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(466, 317);
            this.Controls.Add(this.checkBox1);
            this.Controls.Add(this.label_ver);
            this.Controls.Add(this.tb_preview);
            this.Controls.Add(this.btn_Connect_2);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.cb_COM);
            this.Controls.Add(this.btn_DisConnect);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(4);
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Padding = new System.Windows.Forms.Padding(1);
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "HWiNFO_LCD";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.Form1_FormClosed);
            this.Load += new System.EventHandler(this.Form1_Load_1);
            this.Resize += new System.EventHandler(this.Form1_Resize);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_DisConnect;
        private System.Windows.Forms.NotifyIcon notifyIcon1;
        private System.Windows.Forms.ComboBox cb_COM;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.Button btn_Connect_2;
        private System.Windows.Forms.Timer timer2;
        private System.Windows.Forms.TextBox tb_preview;
        private System.Windows.Forms.Label label_ver;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.CheckBox checkBox1;
    }
}

