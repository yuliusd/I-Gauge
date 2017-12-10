//library
#include <OneWire.h>
#include <DallasTemperature.h>  // DS18B20 library. By : Miles Burton, Tim Newsome, and Guil Barros
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPS++.h>
#include <Adafruit_ADS1015.h>

//PIN SENSOR
#define tegangan 1
#define pressure 3
#define arus 0
#define ONE_WIRE_BUS 30
#define SSpin 53
String ID = "BOGOR10";

//Component Initialization
//LCD 16X2 I2C
#define I2C_ADDR    0x3F // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

//D18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress DS18B20;

//ADS1115
Adafruit_ADS1115 ads;

//RTC
RTC_DS1307 rtc;
DateTime nows;

//GPS
TinyGPSPlus gps;

//SD CARD
File file;
char str[13];
String filename, nama;

//icon
byte water[8] = {
  0x04, 0x04, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E, 0x00,
};
byte pipe[8] = {
  0x0E, 0x04, 0x04, 0x1F, 0x1F, 0x03, 0x00, 0x03,
};
byte smile[8] = {
  0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x11, 0x0E, 0x00,
};
byte clocks1[8] = {
  0x00, 0x00, 0x00, 0x03, 0x0C, 0x09, 0x11, 0x13
};
byte clocks2[8] = {
  0x00, 0x00, 0x00, 0x18, 0x06, 0x02, 0x01, 0x01,
};
byte clocks3[8] = {
  0x10, 0x10, 0x08, 0x0C, 0x03, 0x00, 0x00, 0x00,
};
byte clocks4[8] = {
  0x01, 0x01, 0x02, 0x06, 0x18, 0x00, 0x00, 0x00,
};

//GLOBAL VARIABLE
char g, sdcard[25];
byte a, b, c, t, bulan, hari, jam, menit, detik;
int i, tahun, waktu, kode;
float offset = 0;
int mVperAmp = 185; //185 for 5A, 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500;
unsigned int interval; //menit
unsigned int burst; //second
unsigned long reads = 0; //pressure
unsigned long reads0 = 0; //arus
unsigned long start;
float flat = -987.6543;
float flon = 789.1234;
float hdop=99;
float voltase = 0.0;
float ampere = 0.0;
float tekanan, suhu;
String result, y, operators;
String noHP, source, network, APN, USER, PWD, sms, kuota;

void setup() {
  start = millis();
  Serial.begin(9600);  // Serial USB
  Serial1.begin(9600);  // SIM800L
  Serial2.begin(9600);  // GPS U BLOX
  Serial3.begin(115200); // WEMOS ESP8266

  //LED supply init
  pinMode(32, OUTPUT);  // Green
  pinMode(34, OUTPUT);  // Blue

  //LED status init
  pinMode(36, OUTPUT);  // Red
  pinMode(38, OUTPUT);  // Green
  pinMode(40, OUTPUT);  // Blue

  for (i = 32; i < 41; i++) {
    digitalWrite(i, HIGH);
  }
  delay(2000);
  //INIT ADS1115
  ads.begin();      //ADS1115
  delay(1000);
  //define supply power
  voltase = float(ads.readADC_SingleEnded(tegangan)) * 0.1875 / 1000.0000 * 5.325443787;
  Serial.println(voltase);
  if (voltase >= 8.00) {
    //digitalWrite(34,LOW);
  }
  if (voltase < 8.00) {
    //digitalWrite(32,LOW);
  }
  delay(500);

  //LCD init
  lcd.begin(16, 2);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.createChar(0, water);
  lcd.createChar(1, pipe);
  lcd.createChar(2, clocks1);
  lcd.createChar(3, clocks2);
  lcd.createChar(4, clocks3);
  lcd.createChar(5, clocks4);
  lcd.createChar(6, smile);

  //WELCOME SCREEN
  Serial.println(F("WELCOME"));
  for (i = 0; i <= 2; i++) {
    Serial.println(F("I-GAUGE PDAM BOGOR"));
    digitalWrite(36 + 2 * i, LOW);
    lcd.setCursor(2, 0);
    lcd.print(F("* I-GAUGE"));
    lcd.write(byte(1));
    lcd.print(F(" *"));
    lcd.setCursor(1, 1);
    lcd.write(byte(0));
    lcd.print(F(" PDAM BOGOR "));
    lcd.write(byte(0));
    delay(2000);
    digitalWrite(36 + 2 * i, HIGH);
  }

  ledOff();
  lcd.clear();
  //INISIALISASI RTC
  if (! rtc.begin() || ! rtc.isrunning()) {
    lcd.setCursor(0, 0); lcd.print(F("  RTC ERROR!!!")); //Please run the SetTime
    lcd.setCursor(0, 1); lcd.print(F("   CONTACT CS"));
    Serial.println(F("RTC ERROR!!!"));
    while (1) {
      digitalWrite(36, LOW);  delay(500); //LED RED
      digitalWrite(36, HIGH); delay(500);
    }
  }

  //GET TIME FROM RTC
  for (i = 0; i < 2; i++) {
    digitalWrite(38, LOW); //GREEN
    nows = rtc.now();
    lcd.setCursor(4, 1);  lcd2digits(nows.hour());
    lcd.write(':');       lcd2digits(nows.minute());
    lcd.write(':');       lcd2digits(nows.second());
    lcd.setCursor(3, 0);  lcd2digits(nows.day());
    lcd.write('/');       lcd2digits(nows.month());
    lcd.write('/');       lcd.print(nows.year());
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);   lcd.write(byte(2));
  lcd.setCursor(1, 0);   lcd.write(byte(3));
  lcd.setCursor(0, 1);   lcd.write(byte(4));
  lcd.setCursor(1, 1);   lcd.write(byte(5));
  lcd.setCursor(14, 0);  lcd.write(byte(2));
  lcd.setCursor(15, 0);  lcd.write(byte(3));
  lcd.setCursor(14, 1);  lcd.write(byte(4));
  lcd.setCursor(15, 1);  lcd.write(byte(5));
  lcd.setCursor(6, 0);   lcd.print(F("RTC!"));
  lcd.setCursor(5, 1);   lcd.print(F("READY"));
  lcd.write(byte(6));
  Serial.println(F("RTC OK!!!"));
  //RTC OK - LED CYAN
  digitalWrite(38, LOW); //STATUS OK = GREEN
  digitalWrite(40, LOW); //BLUE
  delay(5000);

  //INIT SD CARD
  ledOff();
  pinMode(SSpin, OUTPUT); //SS MEGA 53, UNO 10
  digitalWrite(SSpin, HIGH);
  digitalWrite(38, LOW); //GREEN
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(F("- SD CARD INIT -"));
  lcd.setCursor(0, 1);
  delay(2000);

  if (!SD.begin(SSpin)) {
    lcd.print(F("SD CARD ERROR!!!"));
    Serial.println(F("SD CARD ERROR!!!"));
    digitalWrite(38, HIGH);
    digitalWrite(36, LOW); //LED RED
    while (1) {}
  }

  lcd.setCursor(0, 1);
  lcd.print(F("- SD CARD OK!! -"));
  Serial.println(F("SD CARD OK!!!"));
  digitalWrite(38, HIGH);
  digitalWrite(36, LOW); // RED
  digitalWrite(40, LOW); // BLUE => PURPLE
  delay(5000);

  //INISIALISASI DS18B20
  lcd.clear();
  ledOff();

  sensors.begin();  //DS18B20
  sensors.getAddress(DS18B20, 0);
  sensors.setResolution(DS18B20, 9); //sensorDeviceAddress, SENSOR_RESOLUTION
  digitalWrite(38, LOW); //STATUS OK = GREEN
  if ( !sensors.getAddress(DS18B20, 0)) {
    lcd.setCursor(0, 0); lcd.print(F("  TEMPERATURE"));
    lcd.setCursor(0, 1); lcd.print(F("  SENSOR ERROR"));
    Serial.println(F("TEMP SENSOR ERROR!!!"));
    digitalWrite(36, LOW); //STATUS RED + GREEN = YELLOW
  }
  else {
    lcd.setCursor(0, 0); lcd.print(F("  TEMPERATURE"));
    lcd.setCursor(0, 1); lcd.print(F("   SENSOR OK!"));
    Serial.println(F("TEMP SENSOR OK!!!"));
  }

  delay(5000);
  ledOff();

  //AMBIL INTERVAL PENGUKURAN
  digitalWrite(38, LOW); //GREEN
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("GET CONFIG.TXT"));
  lcd.setCursor(0, 1);

  //GETTING DATA INTERVAL & PHONE NUMBER
  ledOff();
  configs();

  //TIME INTERVAL
  digitalWrite(38, LOW); //GREEN
  Serial.flush();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("TIME INTERVAL"));
  lcd.setCursor(0, 1);
  lcd.print(interval);
  Serial.print(F("TIME INTERVAL = "));
  Serial.print(interval);
  if (interval == 1) {
    lcd.print(F(" MINUTE"));
    Serial.println(F(" MINUTE"));
  }
  else {
    lcd.print(F(" MINUTES"));
    Serial.println(F(" MINUTES"));
  }
  Serial.flush();
  delay(1000);
  digitalWrite(38, HIGH); //GREEN
  delay(1000);
  digitalWrite(38, LOW); //GREEN
  lcd.clear();

  //BURST INTERVAL
  lcd.print(F("BURST INTERVAL"));
  Serial.print(F("BURST INTERVAL = "));
  lcd.setCursor(0, 1);
  lcd.print(burst);
  Serial.print(burst);
  if (burst == 1) {
    lcd.print(F(" SECOND"));
    Serial.println(F(" SECOND"));
  }
  else {
    lcd.print(F(" SECONDS"));
    Serial.println(F(" SECONDS"));
  }
  Serial.flush();
  delay(1000);
  digitalWrite(38, HIGH); //GREEN
  delay(1000);
  digitalWrite(38, LOW); //GREEN

  //PHONE NUMBER
  lcd.clear();
  lcd.print(F("PHONE NUMBER"));
  lcd.setCursor(0, 1);
  lcd.print(noHP);
  Serial.print(F("No HP = "));
  Serial.println(noHP);
  Serial.flush();
  delay(1000);
  digitalWrite(38, HIGH); //GREEN
  delay(1000);
  digitalWrite(38, LOW); //GREEN

  //INIT SIM800L & SEND SMS
  ledOff();
  lcd.clear();
  lcd.print(F("INIT GSM MODULE"));
  Serial.println(F("INITIALIZATION SIM800L..."));
  Serial.flush();
  Serial1.flush();

  //delay for go to sim800l
  for (i = 0; i < 10; i++) {
    lcd.setCursor(i, 1);
    lcd.print(F("*"));
    Serial.println(i + 1);
    digitalWrite(36, HIGH); // GREEN
    digitalWrite(38, HIGH); // BLUE=> CYAN
    delay(500);
    digitalWrite(36, LOW);
    digitalWrite(38, LOW);
    delay(500);
  }
  ledOff();
  sim800l();
  delay(1000);
  lcd.clear();
  lcd.print(F("CHECK BALANCE"));
  cekkuota();
  lcd.setCursor(0, 1);
  lcd.print(F("FINISH!!!"));

  //INIT GPS
  digitalWrite(47, HIGH);
  ledOff();
  digitalWrite(36, HIGH);//RED
  delay(2000);
  digitalWrite(40, HIGH);//MAGENTA
  delay(2000);
  digitalWrite(36, LOW);//BLUE
  delay(2000);

  pinMode(3, OUTPUT); //activate WEMOS relay
  digitalWrite(3, HIGH);

  Serial.println(F("INITIALIZATION GPS..."));
  Serial.flush();
  Serial2.flush();
  //GPS_ON();

  delay(2000);

  //TURN OFF ALL LED
  ledOff();
  digitalWrite(36, LOW);
  digitalWrite(38, LOW);
  digitalWrite(40, LOW);
  lcd.clear();

  //NYALAKAN WEMOS
  lcd.print(F("----  OPEN  ----"));
  lcd.setCursor(0, 1);
  lcd.print(F("-  WEB SERVER  -"));

  delay(1000);
  Serial.println("BUKA WEBSERVER");
  Serial.println(F("123"));
  Serial.flush();
  Serial3.println(F("123"));

  delay(1000);
  //BIKIN MENU UNTUK WEBSERVER
  while (1) {
    web();
    if (result == "RECORD") break;
  }

  Serial.println("I-GAUGE ready to record data");
  lcd.clear();
  lcd.print(F("SAVE SETTING"));
  //SIMPAN PARAMETER SETTING
  simpanconfigs();
  lcd.setCursor(0, 1);
  lcd.print(F("DONE!!!"));
  delay(1000);
  lcd.clear();

  //SET WAKTU PENGAMBILAN DATA
  waktu = interval * 60;
  nows = rtc.now();
  setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());
  //Serial.print("waktu setup KE SIM800L ");
  //Serial.println(millis() - start);

  //AMBIL DATA
  ambil();
  Alarm.timerRepeat(interval * 60, ambil);
  Alarm.alarmRepeat(5, 0, 0, cekkuota); // 5:00am every day

}

void loop() {
  Alarm.delay(0);
}

void ledOff() {
  digitalWrite(36, HIGH);
  digitalWrite(38, HIGH);
  digitalWrite(40, HIGH);
}

void bersihdata() {
  reads = 0;  reads0 = 0;  tekanan = '0';
  suhu = '0'; voltase = '0';  ampere = '0';
}

void dateTime(uint16_t* date, uint16_t* time) {
  // call back for file timestamps
  DateTime nows = rtc.now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(nows.year(), nows.month(), nows.day());
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(nows.hour(), nows.minute(), nows.second());
}

void lcd2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void save2digits(int number) {
  if (number >= 0 && number < 10) {
    file.print('0');
  }
  file.print(number);
}

void GPS_ON() {
  //INISIALISASI GPS - interval TUNGGU MENCARI SATELIT
  lcd.clear();
  lcd.print(F("--  GPS INIT  --"));
  lcd.setCursor(0, 1);
  lcd.print(F(" Waiting Signal"));
  Serial.flush();
  Serial1.flush();
  Serial2.flush();
  Serial.println(F("Waiting Signal"));
  delay(2000);
  Serial.flush();
  Serial1.flush();
  Serial2.flush();
  //GET COORDINATES
  for (i = 0; i < 60; i++) { //180
    displayInfo();
    if (float(hdop) <= 2.0) break;
  }

  lcd.clear();
  ledOff();

  //DISPLAY LONG & LAT
  digitalWrite(40, LOW);
  lcd.print(F("LONGITUDE"));
  Serial.flush();
  Serial1.flush();
  Serial2.flush();
  delay(2000);
  lcd.setCursor(0, 1);
  if (flon < 0)
    lcd.print("W ");
  else
    lcd.print("E ");
  i = int(abs(flon));
  lcd.print(i); //DERAJAT
  lcd.print(char(223));
  tekanan = ((abs(flon) - float(i)) * 60.0000000); //MENIT
  lcd.print(int(tekanan));
  lcd.print("'");
  tekanan = (tekanan - float(int(tekanan))) * 60.00; //DETIK
  lcd.print(tekanan);
  lcd.print("\"");
  delay(1000);

  lcd.clear();
  lcd.print(F("LATITUDE"));
  Serial.println(" ");
  Serial.print(F("LONGITUDE = "));
  Serial.println(flon, 4);
  Serial.print(F("LATITUDE = "));
  Serial.println(flat, 4);
  Serial.flush();
  Serial2.flush();
  lcd.setCursor(0, 1);
  if (flat < 0)
    lcd.print("S ");
  else
    lcd.print("N ");
  i = int(abs(flat));
  lcd.print(i); //DERAJAT
  lcd.print(char(223));
  tekanan = ((abs(flat) - float(i)) * 60.0000000); //MENIT
  lcd.print(int(tekanan));
  lcd.print("'");
  tekanan = (tekanan - float(int(tekanan))) * 60.00; //DETIK
  lcd.print(tekanan);
  lcd.print("\"");
  delay(2000);
  lcd.clear();
  i = '0';
  tekanan = '0';
}

void displayInfo() {
  digitalWrite(40, LOW); //STATUS BLUE
  start = millis();
  do   {
    while (Serial2.available()) {
      g = Serial2.read();
      gps.encode(g);
      Serial.print(g);
    }
  }
  while (millis() - start < 1000);
  Serial.flush();
  Serial2.flush();
  delay(1000);
  digitalWrite(40, HIGH);
  if (gps.location.isUpdated())  {
    flat = gps.location.lat();
    flon = gps.location.lng();
    hdop = float(gps.hdop.value()) / 100.00;
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(F("- GPS DETECTED -"));
    Serial.println(F("- GPS DETECTED -"));
  }
  if (gps.charsProcessed() < 10)  {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(F("NO GPS DETECTED!"));
    Serial.println(F("NO GPS DETECTED!"));
    ledOff();
    digitalWrite(36, LOW); //RED
    delay(1000);
  }
  Serial.flush();
  Serial2.flush();
  delay(1000);//
}

void ConnectAT(String cmd, int d) {
  i = 0;
  while (1) {
    Serial1.println(cmd);
    while (Serial1.available()) {
      if (Serial1.find("OK"))
        i = 8;
    }
    delay(d);
    if (i > 5) {
      break;
    }
    i++;
  }
  if (i == 8) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("GSM MODULE OK!!");
    Serial.println("OK");
    Serial.flush();
    Serial1.flush();
    result = "OK";
    delay(1000);
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("GSM MODULE ERROR");
    Serial.println("SIM800L ERROR");
    Serial.flush();
    Serial1.flush();
	if(a==5){
    ledOff();
    digitalWrite(36, LOW);
    digitalWrite(40, LOW);
    while (1) {}
	}
  }
}

void ceksim() {
cops:
  filename = "";
  Serial.flush();
  Serial1.flush();
  delay(1000);
  Serial.println(F("AT+COPS?"));
  Serial1.println(F("AT+COPS?"));
  delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+COPS:")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
        Serial.print(g);
      }
    }
  }
  delay(1000);
  Serial.flush();
  Serial1.flush();

  a = filename.indexOf('"');
  b = filename.indexOf('"', a + 1);
  y = filename.substring(a + 1, b);
  if (y == "51089") y = "THREE";

  operators = y;
  //option if not register at network
  if (operators == "")
  {
    goto cops;
  }
  Serial.print(F("OPERATOR="));
  lcd.print(operators);
  Serial.println(operators);
  y = "";
  delay(5000);
}

void sinyal() {
signal:
  filename = "";
  Serial.println(F("AT+CSQ"));
  Serial.print(F("Signal Quality="));
  Serial1.println(F("AT+CSQ"));
  delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+CSQ: ")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
      }
    }
  }
  Serial.flush();
  Serial1.flush();
  delay(500);

  a = (filename.substring(0, filename.indexOf(','))).toInt();
  Serial.print(a);
  Serial.print(" ");
  if (a < 10) {
    lcd.print(F("POOR"));
    Serial.println(F("POOR"));
  }
  if (a > 9 && a < 15) {
    lcd.print(F("FAIR"));
    Serial.println(F("FAIR"));
  }
  if (a > 14 && a < 20) {
    lcd.print(F("GOOD"));
    Serial.println(F("GOOD"));
  }
  if (a > 19 && a <= 31) {
    lcd.print(F("EXCELLENT"));
    Serial.println(F("EXCELLENT"));
  }
  if (a == 99) {
    lcd.print(F("UNKNOWN"));
    Serial.println(F("UNKNOWN"));
    goto signal;
  }
  Serial.flush();
  Serial1.flush();
  delay(5000);
}

void sim800l() { //udah fix
  lcd.clear();
  lcd.print(F("CHECK GSM MODULE"));
  Serial.println(F("CHECK SIM800L"));
  digitalWrite(38, LOW);
  delay(1000);
  digitalWrite(38, HIGH);
  delay(1000);
  
  Serial.println(F("AT+CSCLK=0"));
  Serial1.println(F("AT+CSCLK=0"));
  bacaserial(100);
  digitalWrite(38, LOW);
  delay(1000);
  digitalWrite(38, HIGH);
  delay(1000);
  
  lcd.setCursor(0, 1);
  for(a=0;a<6;a++){
  ConnectAT(F("AT"), 100);
  }
  Serial.flush();
  Serial1.flush();
  digitalWrite(38, LOW);
  delay(1000);
  digitalWrite(38, HIGH);
  delay(1000);
  
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("OPS="));
  ceksim();
  digitalWrite(38, LOW);
  delay(1000);
  digitalWrite(38, HIGH);
  delay(1000);
  
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("SIGNAL="));
  sinyal();
  digitalWrite(38, LOW);
  delay(1000);
  digitalWrite(38, HIGH);
  delay(1000);
  
  Serial.println(F("AT+CMGD=1,4"));
  ConnectAT(F("AT+CMGD=1,4"), 200);
  delay(1000);
  lcd.clear();
  lcd.print(F("SEND SMS"));
  Serial.println(F("SEND SMS"));
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("SMS "));
  Serial.flush();
  Serial1.flush();
  Serial.print(F("AT+CMGF=1 "));
  Serial1.println(F("AT+CMGF=1"));
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  delay(1000);
  Serial.print(F("AT+CSCS=\"GSM\" "));
  Serial.println(F("AT+CSCS=\"GSM\""));
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  delay(1000);
  y = "AT+CMGS=\"" + noHP + "\"";
  Serial.println(y);
  Serial1.println(y);
  while (Serial1.find(">") == false) {
  }
  delay(1000);
  start = millis();
  y = "I-GAUGE ID " + ID + " ready send data to server";
  Serial.println(y);
  Serial1.println(y);
  delay(1000);
  Serial1.println((char)26);

  //WAITING OK
  while (millis() - start <= 60000) {
    digitalWrite(38, HIGH);
    delay(300);
    digitalWrite(38, HIGH);
    delay(300);
  }
  ledOff();
  digitalWrite(38, LOW); //BLUE
  delay(5000);
  digitalWrite(38, HIGH);
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("SMS SENT"));
  Serial.println(F("sms has been sent"));
  Serial.flush();
  Serial1.flush();
  //SIM800L sleep mode
  Serial.println(F("AT+CSCLK=2"));
  Serial1.println(F("AT+CSCLK=2"));
  delay(1000);
}

void configs() {
  a = 0;
  file = SD.open(F("config.txt"));
  lcd.setCursor(0, 1);
  if (file) {
    while (file.available()) {
      g = file.read();
      sdcard[a++] = g;
    }
  }
  else  {
    lcd.print(F("ERROR READING"));
    Serial.println(F("ERROR READING"));
    ledOff();
    while (1) {
      digitalWrite(36, LOW); //YELLOW RED
      digitalWrite(38, LOW);
      delay(1000);
      digitalWrite(38, HIGH);
      delay(1000);
    }
  }
  file.close();

  filename = String(sdcard);
  Serial.println(filename);
  for ( a = 0; a < sizeof(sdcard); a++) {
    sdcard[a] = (char)0;;
    digitalWrite(40, LOW);
    delay(300);
    digitalWrite(40, HIGH);
    delay(200);
  }

  lcd.print(F("FINISH..."));
  a = filename.indexOf("\r");
  interval = filename.substring(0, a).toInt();
  b = filename.indexOf("\r", a + 1);
  burst = filename.substring(a + 1, b).toInt();
  a = filename.indexOf("\r", b + 1);
  noHP = "";//+62
  noHP = filename.substring(b + 2, a);
  filename = '0';
  delay(2000);
}

void simpanconfigs() {
  //file = SD.open("/");
  //file.close();
  //file = SD.open("config.txt");
  //hapus semua data di SD CARD
  //file.close();
  Serial.print("Cek apa ada file config.txt     ");
  Serial.println(SD.exists("/config.txt"));
  delay(1000);
  Serial.print("hapus config.txt    ");
  Serial.println(SD.remove("/config.txt"));
  delay(1000);
  // set date time callback function
  SdFile::dateTimeCallback(dateTime);
  //ISI dengan data baru
  file = SD.open("/config.txt", FILE_WRITE);
  Serial.println(file);
  file.println(interval);
  file.println(burst);
  file.println(noHP);
  file.close();
  Serial.println(F("config has been changed"));

}

void gpsdata() {
  start = millis();
  do   {
    while (Serial2.available()) {
      g = Serial2.read();
      gps.encode(g);
      Serial.print(g);
    }
  }
  while (millis() - start < 1000);
  Serial.flush();
  Serial2.flush();
  if (gps.location.isUpdated())  {
    flat = gps.location.lat();
    flon = gps.location.lng();
    hdop = float(gps.hdop.value()) / 100.00;
  }
  if (gps.charsProcessed() < 10)  {
  }
  }

void ambil() {
  Alarm.delay(0);
  ledOff();
  digitalWrite(38, LOW);//GREEN
  lcd.clear();
  lcd.print(F("--  GET DATA  --"));
  //WAKE UP SIM800L
  Serial.println(F("AT+CSCLK=0"));
  Serial1.println(F("AT+CSCLK=0"));
  delay(200);
  Serial.println(F("AT+CSCLK=0"));
  ConnectAT(F("AT+CSCLK=0"), 200);

  //GET TIME
  nows = rtc.now();
  tahun = nows.year();
  bulan = nows.month();
  hari = nows.day();
  jam = nows.hour();
  menit = nows.minute();
  detik = nows.second();

  lcd.clear();

  //ambil data GPS
  for(i=0;i<2;i++){
	  gpsdata();
  }
  
  //ambil data tekanan, arus, dan voltase
  for (i = 0; i < burst; i++) {
    digitalWrite(38, LOW);
    reads += ads.readADC_SingleEnded(pressure); //tekanan
    reads0 += ads.readADC_SingleEnded(arus); //arus
    nows = rtc.now();
    lcd.setCursor(3, 0);
    lcd.print(nows.year());   lcd.write('/');
    lcd2digits(nows.month()); lcd.write('/');
    lcd2digits(nows.day());
    lcd.setCursor(4, 1);
    lcd2digits(nows.hour());  lcd.write(':');
    lcd2digits(nows.minute());  lcd.write(':');
    lcd2digits(nows.second());
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
  }

  digitalWrite(38, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("VOLTAGE = "));
  lcd.setCursor(0, 1);
  lcd.print(F("CURRENT = "));


  // KONVERSI
  voltase = ((float)reads / (float)burst) * 0.1875 / 1000.0000; // nilai voltase dari nilai DN
  tekanan = (300.00 * voltase - 150.00) * 0.01 + float(offset);
  voltase = (((float)reads0 / (float)burst) * 0.1875);
  voltase = reads0 * 0.1875;
  ampere = ((voltase - ACSoffset) / mVperAmp);
  voltase = voltase / 1000.00;
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);
  voltase = float(ads.readADC_SingleEnded(tegangan)) * 0.1875 / 1000.0000 * 5.325443787;
  if (voltase >= 8.00) {
    digitalWrite(32, HIGH);
    digitalWrite(34, HIGH);
    digitalWrite(34, LOW);
    source = "PLN";
  }
  if (voltase < 8.00) {
    digitalWrite(32, HIGH);
    digitalWrite(34, HIGH);
    digitalWrite(32, LOW);
    source = "BATTERY";
  }

  //tampilkan data ke LCD
  lcd.setCursor(10,0);
  lcd.print(voltase, 2);
  lcd.setCursor(15, 0);
  lcd.print(F("V"));
  lcd.setCursor(10,1);
  lcd.print(ampere, 2);
  lcd.setCursor(15, 1);
  lcd.print(F("A"));
  delay(1000);
  digitalWrite(38, HIGH);
  delay(1000);
  lcd.clear();
  digitalWrite(38, LOW);
  lcd.print(F("PRES = "));
  lcd.print(tekanan, 2);
  lcd.setCursor(13, 0);
  lcd.print(F("bar"));
  lcd.setCursor(0, 1);
  lcd.print(F("TEMP = "));
  lcd.print(suhu, 2);
  lcd.setCursor(13, 1);
  lcd.print(char(223));
  lcd.setCursor(14, 1);
  lcd.print(F("C"));
  delay(500);
  digitalWrite(38, HIGH);
  delay(500);


  //kirim ke server
  Serial.println(F("SEND DATA TO SERVER"));
  server(1);

  //tampilkan ke serial
  Serial.print(tahun);
  Serial.write('-');
  if (bulan < 10) {
    Serial.print("0"); Serial.print(bulan);
  }
  if (bulan >= 10) {
    Serial.print(bulan);
  }
  Serial.print("-");
  if (hari < 10) {
    Serial.print("0"); Serial.print(hari);
  }
  if (hari >= 10) {
    Serial.print(hari);
  }
  Serial.print(" ");
  if (jam < 10) {
    Serial.print("0"); Serial.print(jam);
  }
  if (jam >= 10) {
    Serial.print(jam);
  }
  Serial.print(":");
  if (menit < 10) {
    Serial.print("0"); Serial.print(menit);
  }
  if (menit >= 10) {
    Serial.print(menit);
  }
  Serial.print(":");
  if (detik < 10) {
    Serial.print("0"); Serial.print(detik);
  }
  if (detik >= 10) {
    Serial.print(detik);
  }

  Serial.write('|');
  Serial.print(flon, 4); Serial.print('|');
  Serial.print(flat, 4); Serial.print('|');
  Serial.print(tekanan, 2); Serial.print('|');
  Serial.print(suhu, 2);    Serial.print('|');
  Serial.print(voltase, 2); Serial.print('|');
  Serial.print(ampere, 2);  Serial.print('|');
  Serial.print(source);      Serial.print('|');
  Serial.print(burst);      Serial.print('|');
  Serial.print(interval);   Serial.print('|');
  Serial.print(noHP);       Serial.print('|');
  Serial.print(operators);  Serial.print('|');
  Serial.print(kode);       Serial.print('|');
  Serial.print(network);  Serial.println('|');
  Serial.flush();

  //SAVE DATA
  digitalWrite(38, LOW);
  filename = "";
  filename = String(tahun);
  if (bulan < 10) {
    filename += "0" + String(bulan);
  }
  if (bulan >= 10) {
    filename += String(bulan);
  }
  if (hari < 10) {
    filename += "0" + String(hari);
  }
  if (hari >= 10) {
    filename += String(hari);
  }
  filename += ".ab";

  filename.toCharArray(str, 13);
  i = SD.exists(str);
  // set date time callback function
  SdFile::dateTimeCallback(dateTime);
  file = SD.open(str, FILE_WRITE);
  if (i == 0) {
    file.print(F("DATE (YYYY-MM-DD HH:MM:SS)| LONGITUDE (DD.DDDDDD°) | LATITUDE (DD.DDDDDD°) | "));
    file.print(F("PRESSURE (BAR) | TEMPERATURE (°C) | VOLTAGE (VOLT)| CURRENT (AMPERE) | SOURCE | "));
    file.println(F("BURST INTERVAL | DATA INTERVAL | PHONE NUMBER | OPERATOR | SERVER CODE | NETWORK"));

  }
  //simpan data ke SD CARD
  //FORMAT DATA = DATE | LONGITUDE | LATITUDE | PRESSURE | TEMPERATURE | VOLTAGE | CURRENT | SOURCE | 
  //            BURST INTERVAL | DATA INTERVAL | PHONE NUMBER | OPERATOR | SERVER CODE | NETWORK
  file.print(tahun);  file.print('-');
  save2digits(bulan); file.print('-');
  save2digits(hari);  file.print(' ');
  save2digits(jam);   file.print(':');
  save2digits(menit); file.print(':');
  save2digits(detik); file.print('|');
  file.print(flon, 4);    file.print('|');
  file.print(flat, 4);    file.print('|');
  file.print(tekanan, 2);   file.print('|');
  file.print(suhu, 2);    file.print('|');
  file.print(voltase, 2);   file.print('|');
  file.print(ampere, 2);  file.print('|');
  file.print(source);      file.print('|');
  file.print(burst);      file.print('|');
  file.print(interval);   file.print('|');
  file.print(noHP);     file.print('|');
  file.print(operators);  file.print('|');
  file.print(kode); file.print('|');
  file.print(network);  file.println('|');
  file.flush();
  file.close();
  //bersih variabel
  bersihdata();
  digitalWrite(38, HIGH);
  Serial.println(F("AT+CSCLK=2"));
  Serial1.println(F("AT+CSCLK=2"));
  delay(1000);
  digitalWrite(38, LOW);
}

void web() {
  while (Serial3.find("Go") == false) {
  }

  Serial.println(F("send data to webserver"));
  Serial.println("&");
  Serial.flush();
  Serial3.println("&");
  Serial3.flush();

  while (Serial3.find("%") == false) {
  }

  //ambil data
  nows = rtc.now();
  tahun = nows.year();
  bulan = nows.month();
  hari = nows.day();
  jam = nows.hour();
  menit = nows.minute();
  detik = nows.second();
  reads = ads.readADC_SingleEnded(pressure);
  voltase = reads * 0.1875 / 1000.0000;           // nilai voltase dari nilai DN
  tekanan = (300.00 * voltase - 150.00) * 0.01 + float(offset); // bar
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);
  reads0 = ads.readADC_SingleEnded(arus);
  voltase = reads0 * 0.1875;
  ampere = ((voltase - ACSoffset) / mVperAmp);
  voltase = float(ads.readADC_SingleEnded(tegangan)) * 0.1875 / 1000.0000 * 5.325443787;

  if (operators == "") {
    operators = "NONE";
  }
  //FORMAT DATA = ID | LONGITUDE | LATITUDE | DATE | PRESSURE | TEMPERATURE | VOLTAGE | CURRENT | BURST INTERVAL | DATA INTERVAL | PHONE NUMBER | OPERATOR | offset
  result = "";
  result = ID + "|" + String(flon, 4) + "|" + String(flat, 4) + "|";
  result += String(tahun) + "/";
  if (bulan < 10) {
    result += "0" + String(bulan) + "/";
  }
  if (bulan >= 10) {
    result += String(bulan) + "/";
  }
  if (hari < 10) {
    result += "0" + String(hari) + " ";
  }
  if (hari >= 10) {
    result += String(hari) + " ";
  }
  if (jam < 10) {
    result += "0" + String(jam) + ":";
  }
  if (jam >= 10) {
    result += String(jam) + ":";
  }
  if (menit < 10) {
    result += "0" + String(menit) + ":";
  }
  if (menit >= 10) {
    result += String(menit) + ":";
  }
  if (detik < 10) {
    result += "0" + String(detik);
  }
  if (detik >= 10) {
    result += String(detik);
  }
  result += "|" + String(tekanan, 2) + "|" + String(suhu, 2) + "|" + String(voltase, 2) + "|" + String(ampere, 2) + "|" + String(burst) + "|" + String(interval) + "|" + noHP + "|" + operators + "|" + offset ;
  result += "*";
  y = "";

  Serial.println(result);
  Serial.flush();
  Serial3.print(result);
  Serial3.flush();
  start = millis();
  while ((start + 2000) > millis()) {
    while (Serial3.available() > 0) {
      g = Serial3.read();
      y += g;
      if (g == '#')break;
    }
  }
  delay(1000);
  Serial.print("DATA=");
  Serial.println(y);
  Serial.flush();
  result = "";
  b = y.indexOf('^');
  a = y.indexOf('|');
  flon = y.substring(b + 1, a).toFloat();
  b = y.indexOf('|', a + 1);
  flat = y.substring(a + 1, b).toFloat();
  a = y.indexOf('|', b + 1);
  burst = y.substring(b + 1, a).toInt();
  b = y.indexOf('|', a + 1);
  interval = y.substring(a + 1, b).toInt();
  a = y.indexOf('|', b + 1);
  noHP = y.substring(b + 1, a);
  b = y.indexOf('|', a + 1);
  offset = y.substring(a + 1, b).toFloat();
  a = y.indexOf('#', b + 1);
  result = y.substring(b + 1, a);

  Serial.print("flon=");
  Serial.println(flon, 4);
  Serial.print("flat=");
  Serial.println(flat, 4);
  Serial.print("Burst1=");
  Serial.println(burst);
  Serial.print("Interval1=");
  Serial.println(interval);
  Serial.print("NoHP1=");
  Serial.println(noHP);
  Serial.print("OFFSET=");
  Serial.println(offset);
  Serial.print("Kirim1=");
  Serial.println(result);
  Serial.flush();

  bersihdata();
}

void bacaserial(int wait) {
  start = millis();
  while ((start + wait) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.print(g);
    }
  }
}

void server(byte t) {
  //CHECK GPRS ATTACHED OR NOT
serve:
  filename = "";
  //operators="TELKOMSEL";
  Serial.print(F("AT+CGATT? "));
  Serial1.println(F("AT+CGATT?"));
  delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+CGATT: ")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
      }
    }
  }
  Serial.println(filename);
  if (filename.toInt() == 1) {
    //kirim data ke server
	ledOff();
	digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    Serial.println(F("AT+CIPSHUT"));
    Serial1.println(F("AT+CIPSHUT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    //ATUR APN SESUAI DENGAN PROVIDER
    apn(operators);
    //CONNECTION TYPE
    Serial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    Serial1.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    //APN ID
    result = "AT+SAPBR=3,1,\"APN\",\"" + APN + "\"";
    Serial.println(result);
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    //APN USER
    result = "AT+SAPBR=3,1,\"USER\",\"" + USER + "\"";
    Serial.println(result);
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    //APN PASSWORD
    result = "AT+SAPBR=3,1,\"PWD\",\"" + PWD + "\"";
    Serial.println(result);
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    //OPEN BEARER
    start = millis();
    Serial.println(F("AT+SAPBR=1,1"));
    Serial1.println(F("AT+SAPBR=1,1"));
    bacaserial(1000);
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    //QUERY BEARER
    Serial.println(F("AT+SAPBR=2,1"));
    Serial1.println(F("AT+SAPBR=2,1"));
    start = millis();
    while (start + 5000 > millis()) {
      while (Serial1.available() > 0) {
        Serial.print(Serial1.read());
        if (Serial1.find("OK")) {
          i = 5;
          break;
        }
      }
      if (i == 5) {
        break;
      }
    }
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    //TERMINATE HTTP SERVICE
    Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    delay(500);
    //INITIALIZE HTTP SERVICE
    Serial.println(F("AT+HTTPINIT"));
    Serial1.println(F("AT+HTTPINIT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    delay(500);
    //SET HTTP PARAMETERS VALUE
    Serial.println(F("AT+HTTPPARA=\"CID\",1"));
    Serial1.println(F("AT+HTTPPARA=\"CID\",1"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    delay(500);
    if (t == 1) {
      //SET HTTP URL
      Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      bacaserial(1000);
      Serial.flush();
      Serial1.flush();
      //http://www.mantisid.id/api/product/pdam_dt_c.php?="Data":"'2017-11-05 10:00:00', '111.111111', '-6.2222222', '400.33', '34.00', '5.05', '1.66', 'pdam_001', '5', '3'"
      //Formatnya Date, longitude, latitude, pressure, temperature, volt, ampere, source, id, burst interval, data interval
      y = "{\"Data\":\"'";
      y += String(nows.year()) + "-";
      if (bulan < 10) {
        y += "0" + String(bulan) + "-";
      }
      if (bulan >= 10) {
        y += String(bulan) + "-";
      }
      if (hari < 10) {
        y += "0" + String(hari) + " ";
      }
      if (hari >= 10) {
        y += String(hari) + " ";
      }
      if (jam < 10) {
        y += "0" + String(jam) + ":";
      }
      if (jam >= 10) {
        y += String(jam) + ":";
      }
      if (menit < 10) {
        y += "0" + String(menit) + ":";
      }
      if (menit >= 10) {
        y += String(menit) + ":";
      }
      if (detik < 10) {
        y += "0" + String(detik);
      }
      if (detik >= 10) {
        y += String(detik);
      }
      y += "','";
      y += String(flon, 4) + "','";
      y += String(flat, 4) + "','";
      y += String(tekanan, 2) + "','";
      y += String(suhu, 2) + "','";
      y += String(voltase, 2) + "','";
      y += String(ampere, 2) + "','";
      y += String(source) + "','";
      y += String(ID) + "','";
      y += String(burst) + "','";
      y += String(interval) + "'\"}";
    }
    if (t == 2) {
      //GET TIME
      nows = rtc.now();
      tahun = nows.year();
      bulan = nows.month();
      hari = nows.day();
      jam = nows.hour();
      menit = nows.minute();
      detik = nows.second();

      //SET HTTP URL
      Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_sim_c.php\""));
      Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_sim_c.php\""));
      bacaserial(1000);
      Serial.flush();
      Serial1.flush();
      //http://www.mantisid.id/api/product/pdam_sim_c.php
      //Formatnya YYY-MM-DD HH:MM:SS, ID, PULSA, KUOTA
      y = "{\"Data\":\"'";
      y += String(nows.year()) + "-";
      if (bulan < 10) {
        y += "0" + String(bulan) + "-";
      }
      if (bulan >= 10) {
        y += String(bulan) + "-";
      }
      if (hari < 10) {
        y += "0" + String(hari) + " ";
      }
      if (hari >= 10) {
        y += String(hari) + " ";
      }
      if (jam < 10) {
        y += "0" + String(jam) + ":";
      }
      if (jam >= 10) {
        y += String(jam) + ":";
      }
      if (menit < 10) {
        y += "0" + String(menit) + ":";
      }
      if (menit >= 10) {
        y += String(menit) + ":";
      }
      if (detik < 10) {
        y += "0" + String(detik);
      }
      if (detik >= 10) {
        y += String(detik);
      }
      y += "','";
      y += String(ID) + "','";
      y += String(sms) + "','";
      y += String(kuota) + "'\"}";

      //simpan data sisa pulsa dan kuota ke dalam sd card
      result = "pulsa.ab";

      result.toCharArray(str, 13);
      // set date time callback function
      SdFile::dateTimeCallback(dateTime);
      file = SD.open(str, FILE_WRITE);
      file.println(y);
      file.flush();
      file.close();
    }
    //SET HTTP DATA FOR SENDING TO SERVER
    result = "AT+HTTPDATA=" + String(y.length() + 1) + ",15000";
    Serial.println(result);
    Serial1.println(result);
    while (Serial1.available() > 0) {
      while (Serial1.find("DOWNLOAD") == false) {
      }
    }

    //SEND DATA
    Serial.println(y);
    Serial1.println(y);
    Serial.flush();
    Serial1.flush();
    bacaserial(1000);

    //HTTP METHOD ACTION
    filename = "";
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    start = millis();
    Serial.println(F("AT+HTTPACTION=1"));
    Serial1.println(F("AT+HTTPACTION=1"));
    while (Serial1.available() > 0) {
      while (Serial1.find("OK") == false) {
        if (Serial1.find("ERROR")) {
          goto serve;
        }
      }
    }
    Serial.flush();
    Serial1.flush();
    a = '0';
    b = '0';
    //CHECK KODE HTTPACTION
    while ((start + 20000) > millis()) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
        Serial.print(g);
        a = filename.indexOf(":");
        b = filename.length();
        if (b - a > 12)break;
      }
      if (b - a > 12) break;
    }
    Serial.println();
    Serial.flush();
    Serial1.flush();
    a = '0';
    b = '0';
    delay(500);
    Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    digitalWrite(38,LOW);
    delay(500);
    digitalWrite(38, HIGH);
    delay(500);
    Serial.println(F("AT+SAPBR=0,1"));
    Serial1.println(F("AT+SAPBR=0,1"));
    while (start + 10000 > millis()) {
      while (Serial1.available() > 0) {
        if (Serial1.find("OK")) {
          a = 1;
          break;
        }
      }
      if (a = 1) break;
    }
    a = '0';
    Serial.println("");
    Serial.println(filename);
    Serial.flush();
    Serial1.flush();
    a = filename.indexOf(',');
    b = filename.indexOf(',', a + 1);
    kode = filename.substring(a + 1, b).toInt();
    statuscode(kode);
    Serial.flush();
    Serial1.flush();
  }
  else {
    network = "Error";
    kode = 999;
  }
}

void apn(String nama) {
  if (nama == "TELKOMSEL") {
    APN = "Telkomsel";
    USER = "";
    PWD = "";
  }
  if (nama == "INDOSAT") {
    APN = "indosatgprs";
    USER = "indosat";
    PWD = "indosatgprs";
  }
  if (nama == "EXCELCOM") {
    APN = "internet";
    USER = "";
    PWD = "";
  }
  if (nama == "THREE") {
    APN = "3data";
    USER = "3data";
    PWD = "3data";
  }
}

void statuscode(int w) {
  if (w == 100) {
    network = "Continue";
  }
  if (w == 101) {
    network = "Switching Protocols";
  }
  if (w == 200) {
    network = "OK";
  }
  if (w == 201) {
    network = "Created";
  }
  if (w == 202) {
    network = "Accepted";
  }
  if (w == 203) {
    network = "Non-Authoritative Information";
  }
  if (w == 204) {
    network = "No Content";
  }
  if (w == 205) {
    network = "Reset Content";
  }
  if (w == 206) {
    network = "Partial Content";
  }
  if (w == 300) {
    network = "Multiple Choices";
  }
  if (w == 301) {
    network = "Moved Permanently";
  }
  if (w == 302) {
    network = "Found";
  }
  if (w == 303) {
    network = "See Other";
  }
  if (w == 304) {
    network = "Not Modified";
  }
  if (w == 305) {
    network = "Use Proxy";
  }
  if (w == 307) {
    network = "Temporary Redirect";
  }
  if (w == 400) {
    network = "Bad Request";
  }
  if (w == 401) {
    network = "Unauthorized";
  }
  if (w == 402) {
    network = "Payment Required";
  }
  if (w == 403) {
    network = "Forbidden";
  }
  if (w == 404) {
    network = "Not Found";
  }
  if (w == 405) {
    network = "Method Not Allowed";
  }
  if (w == 406) {
    network = "Not Acceptable";
  }
  if (w == 407) {
    network = "Proxy Authentication Required";
  }
  if (w == 408) {
    network = "Request Time-out";
  }
  if (w == 409) {
    network = "Conflict";
  }
  if (w == 410) {
    network = "Gone";
  }
  if (w == 411) {
    network = "Length Required";
  }
  if (w == 412) {
    network = "Precondition Failed";
  }
  if (w == 413) {
    network = "Request Entity Too Large";
  }
  if (w == 414) {
    network = "Request-URI Too Large";
  }
  if (w == 415) {
    network = "Unsupported Media Type";
  }
  if (w == 416) {
    network = "Requested range not satisfiable";
  }
  if (w == 417) {
    network = "Expectation Failed ";
  }
  if (w == 500) {
    network = "Internal Server Error";
  }
  if (w == 501) {
    network = "Not Implemented";
  }
  if (w == 502) {
    network = "Bad Gateway";
  }
  if (w == 503) {
    network = "Service Unavailable";
  }
  if (w == 504) {
    network = "Gateway Time-out";
  }
  if (w == 505) {
    network = "HTTP Version not supported";
  }
  if (w == 600) {
    network = "Not HTTP PDU";
  }
  if (w == 601) {
    network = "Network Error";
  }
  if (w == 602) {
    network = "No memory";
  }
  if (w == 603) {
    network = "DNS Error";
  }
  if (w == 604) {
    network = "Stack Busy";
  }
}

void cekkuota() {
  c = 0;
  Serial.flush();
  Serial1.flush();
  Serial.println("AT");
  Serial1.println("AT");
  Serial.flush();
  Serial1.flush();
  bacaserial(100);
  delay(500);
  Serial.println("AT+CSCLK=0");
  Serial1.println("AT+CSCLK=0");
  Serial.flush();
  Serial1.flush();
  delay(500);
  Serial.println("AT+CSCLK=0");
  Serial1.println("AT+CSCLK=0");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  delay(500);
top:
  if (c == 5)
  {
    sms = "sisa pulsa tidak diketahui";
    kuota = "sisa kuota tidak diketahui";
    //goto down;
  }
  sms = "";
  kuota = "";

  start = millis();
  Serial.println("AT+COPS?");
  Serial1.println("AT+COPS?");
  Serial.flush();
  Serial1.flush();
  while ((start + 1000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      sms += g;

    }
  }

  Serial.flush();
  Serial1.flush();
  a = sms.indexOf(',');
  if (a == 0) {
    goto top;
  }

  Serial.println("AT+CSQ");
  Serial1.println("AT+CSQ");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();

  Serial.println("AT+CUSD=2");
  Serial1.println("AT+CUSD=2");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  delay(400);

  Serial.println("AT+CUSD=1");
  Serial1.println("AT+CUSD=1");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  delay(400);
  sms = "";
  start = millis();
  Serial.println("AT+CUSD=1,\"*888#\"");
  Serial1.println("AT+CUSD=1,\"*888#\"");
  Serial.flush();
  Serial1.flush();

  while ((start + 5000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      sms += g;
    }
  }
  Serial.flush();
  Serial1.flush();
  a = sms.indexOf(':');
  b = sms.indexOf('"', a + 1);
  i = sms.substring(a + 1, b - 1).toInt();
  Serial.print("+CUSD: ");
  Serial.println(i);
  if (i == 0) {
    c++;
    goto top;
  }
  a = sms.indexOf(':');
  b = sms.indexOf('.', a + 1);
  b = sms.indexOf('.', b + 1);
  b = sms.indexOf('.', b + 1);
  sms = sms.substring(a + 5, b);

  delay(100);
  start = millis();
  Serial.println("AT+CUSD=1,\"3\"");
  Serial1.println("AT+CUSD=1,\"3\"");
  while ((start + 3000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      kuota += g;
    }
  }
  Serial.flush();
  Serial1.flush();
  a = kuota.indexOf(':');
  b = kuota.indexOf('"', a + 1);
  i = kuota.substring(a + 1, b - 1).toInt();
  if (i == 0) {
    goto top;
  }
  delay(100);

  kuota = "";
  start = millis();
  Serial.println("AT+CUSD=1,\"2\"");
  Serial1.println("AT+CUSD=1,\"2\"");
  while ((start + 3000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      kuota += g;
    }
  }
  Serial.flush();
  Serial1.flush();
  a = kuota.indexOf(':');
  b = kuota.indexOf('"', a + 1);
  i = kuota.substring(a + 1, b - 1).toInt();
  if (i == 0) {
    goto top;
  }
  a = kuota.indexOf(':');
  b = kuota.indexOf('.', a + 1);
  kuota = kuota.substring(a + 5, b);

down:
  a = 0; b = 0; i = 0;
  Serial.flush();
  Serial1.flush();
  Serial.println("AT+CUSD=2");
  Serial1.println("AT+CUSD=2");
  bacaserial(200);
  Serial.flush();
  Serial1.flush();
  Serial.println();
  Serial.println(sms);
  Serial.println(kuota);
  server(2); //send kuota to server
}
