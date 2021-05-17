#include <Wire.h> 
#include "LiquidCrystal_I2C.h"
// https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library

#define MAX_COL 20
#define MAX_ROW 4

LiquidCrystal_I2C lcd(0x27, MAX_COL, MAX_ROW);
int inByte = 0;         // incoming serial byte
int inLine = 0;
bool Flag_Ready = false;
char ver[20]= "2021/05/17";

byte myChar1[8] = {B00000, B10000, B10000, B10000, B10000, B10000, B00000};
byte myChar2[8] = {B00000, B11000, B11000, B11000, B11000, B11000, B00000};
byte myChar3[8] = {B00000, B11100, B11100, B11100, B11100, B11100, B00000};
byte myChar4[8] = {B00000, B11110, B11110, B11110, B11110, B11110, B00000};
byte myChar5[8] = {B00000, B11111, B11111, B11111, B11111, B11111, B00000};

void setup(){
  Serial.begin(9600);
	lcd.begin();
  lcd.createChar(0, myChar1);
  lcd.createChar(1, myChar2);
  lcd.createChar(2, myChar3);
  lcd.createChar(3, myChar4);
  lcd.createChar(4, myChar5);
  delay(10);
	lcd.backlight();
  delay(10);
  lcd.setCursor(0, 0);
	lcd.print("USB Connecting..");
  while (!Serial) {;}
  ChangeLine();
  lcd.print("USB Ready");
  ChangeLine();
  ChangeLine();
  lcd.print("ver: ");lcd.print(ver);  
  Flag_Ready = true;
}

void loop(){
  if (Serial.available() > 0) {    
    if(Flag_Ready) {  //針對剛開機時的情況
      Flag_Ready = false;
      Clear();
    } 
    
    inByte = Serial.read();
    delay(1);
    if(inByte=='\n'){ ChangeLine();}
    else if(inByte=='\''){ lcd.print(char(223));}
    else if(inByte==23){ lcd.write(byte(0));}
    else if(inByte==24){ lcd.write(byte(1));}
    else if(inByte==25){ lcd.write(byte(2));}
    else if(inByte==26){ lcd.write(byte(3));}
    else if(inByte==27){ lcd.write(byte(4));}
    else if(inByte==28){ lcd.backlight();}
    else if(inByte==29){ lcd.noBacklight();}  
    else if(inByte==30){ ChangeLine();}
    else if(inByte==31){ Clear();}
    else{lcd.print(char(inByte));}
  }
}

void ChangeLine(){
  inLine++;
  if(inLine >= MAX_ROW) inLine=0;
  lcd.setCursor(0, inLine);
}

void Clear(){
  for(int i=0; i<MAX_ROW; i++){
    lcd.setCursor(0, i);
    for(int j=0; j<MAX_COL; j++){
      lcd.print(" ");
    }
  }
  lcd.setCursor(0, 0);
  inLine=0;
}
