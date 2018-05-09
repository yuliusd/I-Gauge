//LIBRARY
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
#include <Narcoleptic.h>
#include <avr/power.h>

//Component Initialization
String ID = "BOGOR04";
#define I2C_ADDR    0x27 //0X3F
#define ONE_WIRE_BUS 43
#define ads_addr 0x48
#define rtc_addr 0x68
#define arus 0
#define tegangan 1
#define pressure 3
#define GPower 31
#define BPower 29
#define RState 27
#define GState 25
#define BState 23
#define SCKpin 52
#define MISOpin 50
#define MOSIpin 51
#define SSpin 53

//LCD 1602 I2C
#define BACKLIGHT_PIN   3
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
Adafruit_ADS1115 ads(ads_addr);

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
byte a, b, c, bulan, hari, jam, menit, detik;
int i, tahun, kode;
int mVperAmp = 185; //185 for 5A, 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500;
unsigned int interval; //menit
unsigned int burst; //second
unsigned long reads = 0; //pressure
unsigned long reads0 = 0; //volt & arus
unsigned long start, waktu;
float offset = 0;
float flat = -98.76543;
float flon = 789.1234;
float hdop;
float voltase = 0.00;
float ampere = 0.00;
float tekanan, suhu;
String result, y, source;
String teks, operators;
String network, APN, USER, PWD, sms, kuota, noHP;
byte  keep_SPCR;

void setup() {
  Serial.begin(9600);  // Serial USB
  Serial1.begin(9600);  // SIM800L

  // disable unused pin function
  ADCSRA = 0; //ADC OFF
  keep_SPCR = SPCR;
  off();

  //LCD init
  s_on();
  Serial.println(F("INIT LCD"));
  Serial.flush();
  s_off();
  i_En(I2C_ADDR);
  lcd.begin(16, 2);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.createChar(0, water);
  lcd.createChar(1, pipe);
  lcd.createChar(2, clocks1);
  lcd.createChar(3, clocks2);
  lcd.createChar(4, clocks3);
  lcd.createChar(5, clocks4);
  i_Dis();
  a = ID.indexOf('R');
  nama = ID.substring(a + 1);

  //WELCOME SCREEN
  for (i = 1; i <= 3; i++) {
    digitalWrite(BState, HIGH);
    s_on();
    Serial.println(F("I-GAUGE PDAM BOGOR"));
    Serial.flush();
    s_off();
    i_En(I2C_ADDR);
    lcd.setCursor(1, 0);
    lcd.print(F("I-GAUGE "));
    lcd.write(byte(1));
    lcd.print(F(" PDAM"));
    lcd.setCursor(2, 1);
    lcd.write(byte(0));
    lcd.print(F(" BOGOR "));
    lcd.print(nama);
    lcd.print(F(" "));
    lcd.write(byte(0));
    i_Dis();
    Narcoleptic.delay(1000);
    digitalWrite(BState, LOW);
  }

  s_on();
  Serial.println(F("I-GAUGE"));
  Serial.flush();
  s_off();

  //LED status init
  pinMode(RState, OUTPUT);  // Red
  pinMode(GState, OUTPUT);  // Green
  pinMode(BState, OUTPUT);  // Blue
  pinMode(GPower, OUTPUT);  // Blue
  pinMode(BPower, OUTPUT);  // Blue

  for (i = 1; i <= 5; i++) {
    digitalWrite(21 + i * 2, LOW);
  }

  //INIT ADS1115
  i_En(ads_addr);
  s_on();
  Serial.println(F("INIT ADS1115"));
  Serial.flush();
  s_off();
  i_Dis();

  i_En(I2C_ADDR);
  lcd.clear();
  i_En(rtc_addr);
  //INISIALISASI RTC
  if (! rtc.begin() || ! rtc.isrunning()) {
    lcd.setCursor(0, 0); lcd.print(F("  RTC ERROR!!!")); //Please run the SetTime
    lcd.setCursor(0, 1); lcd.print(F("   CONTACT CS"));
    i_Dis();
    s_off();
    Serial.println(F("RTC ERROR!!!"));
    Serial.flush();
    s_off();
    off();
    while (1) {
      digitalWrite(RState, HIGH);  Narcoleptic.delay(500); //LED RED
      digitalWrite(RState, LOW);   Narcoleptic.delay(500);
    }
  }

  //GET TIME FROM RTC
  for (i = 0; i < 3; i++) {
    digitalWrite(GState, HIGH);
    i_Dis();
    i_En(rtc_addr);
    nows = rtc.now();
    i_Dis();
    i_En(I2C_ADDR);
    lcd.setCursor(0, 0);   lcd.write(byte(2));
    lcd.setCursor(1, 0);   lcd.write(byte(3));
    lcd.setCursor(0, 1);   lcd.write(byte(4));
    lcd.setCursor(1, 1);   lcd.write(byte(5));
    lcd.setCursor(14, 0);  lcd.write(byte(2));
    lcd.setCursor(15, 0);  lcd.write(byte(3));
    lcd.setCursor(14, 1);  lcd.write(byte(4));
    lcd.setCursor(15, 1);  lcd.write(byte(5));
    lcd.setCursor(4, 1);  lcd2digits(nows.hour());
    lcd.write(':');       lcd2digits(nows.minute());
    lcd.write(':');       lcd2digits(nows.second());
    lcd.setCursor(3, 0);  lcd2digits(nows.day());
    lcd.write('/');       lcd2digits(nows.month());
    lcd.write('/');       lcd.print(nows.year());
    Narcoleptic.delay(500);
    digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
  }

  s_on();
  Serial.println(F("RTC OK!!!"));
  Serial.flush();
  s_off();
  pinMode(SSpin, OUTPUT);
  digitalWrite(SSpin, HIGH);
  power_timer0_enable();
  delay(500);

  //INIT SD CARD
  i_En(I2C_ADDR);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("- SD CARD INIT -"));
  spiEn();
  delay(100);
  if (!SD.begin(SSpin)) { //SD CARD ERROR
    i_En(I2C_ADDR);
    lcd.setCursor(0, 1);
    lcd.print(F("SD CARD ERROR!!!"));
    s_on();
    Serial.println(F("SD CARD ERROR!!!"));
    Serial.flush();
    s_off();
    off();
    digitalWrite(RState, HIGH); //LED RED
    while (1) {}
  }
  spiDis();
  i_En(I2C_ADDR);
  lcd.setCursor(0, 1);
  lcd.print(F("- SD CARD OK!! -"));
  i_Dis();
  s_on();
  Serial.println(F("SD CARD OK!!!"));
  Serial.flush();
  s_off();
  power_timer0_enable();

  //INISIALISASI DS18B20
  for (i = 1; i <= 2; i++) {
    digitalWrite(BState, HIGH);
    delay(300);
    digitalWrite(BState, LOW);
    delay(500);
  }
  i_En(I2C_ADDR);
  lcd.clear();
  ledOff();
  sensors.begin();  //DS18B20
  sensors.getAddress(DS18B20, 0);
  sensors.setResolution(DS18B20, 9); //sensorDeviceAddress, SENSOR_RESOLUTION
  if ( !sensors.getAddress(DS18B20, 0)) {
    lcd.setCursor(0, 0); lcd.print(F("  TEMPERATURE"));
    lcd.setCursor(0, 1); lcd.print(F("  SENSOR ERROR"));
    s_on();
    Serial.println(F("TEMP SENSOR ERROR!!!"));
    Serial.flush();
    s_off();
    digitalWrite(RState, HIGH);
  }
  else {
    lcd.setCursor(0, 0); lcd.print(F("  TEMPERATURE"));
    lcd.setCursor(0, 1); lcd.print(F("   SENSOR OK!"));
    digitalWrite(GState, HIGH);
    s_on();
    Serial.println(F("TEMP SENSOR OK!!!"));
    Serial.flush();
    s_off();
  }
  i_Dis();
  Narcoleptic.delay(500);
  ledOff();

  //AMBIL INTERVAL PENGUKURAN
  digitalWrite(RState, HIGH);
  digitalWrite(GState, HIGH);
  i_En(I2C_ADDR);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("GET CONFIG.TXT"));
  lcd.setCursor(0, 1);
  i_Dis();
  configs();
  ledOff();

  //TIME INTERVAL
  digitalWrite(GState, HIGH);
  digitalWrite(BState, HIGH);
  i_En(I2C_ADDR);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("TIME INTERVAL"));
  lcd.setCursor(0, 1);
  lcd.print(interval);
  s_on();
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
  off();
  Narcoleptic.delay(2000);
  ledOff();

  //BURST INTERVAL
  i_En(I2C_ADDR);
  lcd.clear();
  digitalWrite(GState, HIGH);
  digitalWrite(BState, HIGH);
  lcd.print(F("BURST INTERVAL"));
  s_on();
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
  off();
  ledOff();

  //INIT SIM800L
  s_on();
  Serial.println(F("INITIALIZATION SIM800L..."));
  Serial.flush();
  off();

  power_timer0_enable();
  digitalWrite(BState, HIGH);
  delay(500);
  digitalWrite(GState, HIGH);
  delay(1000);
  ledOff();
  sim800l();
  Narcoleptic.delay(1000);

  //CEK KUOTA
  power_timer0_enable();
  digitalWrite(BState, HIGH);
  delay(500);
  digitalWrite(RState, HIGH);
  delay(1000);
  ledOff();
  off();
  cekkuota();
  i_En(I2C_ADDR);
  lcd.setCursor(0, 1);
  lcd.print(F("FINISH!!!"));
  off();
  Narcoleptic.delay(1000);

  bersihdata();
  s_on();
  Serial.println(F("I-GAUGE READY TO RECORD DATA"));
  Serial.flush();
  s_off();
  off();

  //interval=3;
  //SET WAKTU PENGAMBILAN DATA
  i_En(rtc_addr);
  nows = rtc.now();
  setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());
  Alarm.timerRepeat(interval * 60, ambil);
  Alarm.alarmRepeat(4, 59, 59, cekkuota); // 5:00am every day

  //AMBIL DATA
  ambil();
}

void loop() {
  Alarm.delay(0);
}

void ambil() {
  Alarm.delay(0);
  ledOff();
  digitalWrite(BState, HIGH);
  i_En(I2C_ADDR);
  lcd.clear();
  lcd.print(F("--  GET DATA  --"));
  i_Dis();

  //WAKE UP SIM800L
  power_timer0_enable();
  s_on();
  s1_on();
  Serial.println(F(" "));
  Serial.println(F("AT+CSCLK=0"));
  Serial1.println(F("AT+CSCLK=0"));
  Serial1.flush();
  s_off();
  s1_off();
  delay(500);
  s_on();
  s1_on();
  Serial1.println(F("AT+CSCLK=0"));
  Serial1.flush();
  bacaserial(200);
  s_off();
  s1_off();
  delay(500);
  

  //GET TIME
  i_En(rtc_addr);
  nows = rtc.now();
  waktu = nows.unixtime();
  tahun = nows.year();
  bulan = nows.month();
  hari = nows.day();
  jam = nows.hour();
  menit = nows.minute();
  detik = nows.second();
  lcd.clear();
  i_Dis();

  s_on();
  Serial.print(nows.year());   Serial.write('/');
  Serial.print(nows.month()); Serial.print('/');
  Serial.print(nows.day());
  Serial.print(" ");
  Serial.print(nows.hour());  Serial.print(':');
  Serial.print(nows.minute());  Serial.print(':');
  Serial.println(nows.second());
  Serial.flush();
  s_off();

  //get data pressure, current, dan voltage
  for (i = 0; i < burst * 10; i++) {
    i_En(ads_addr);
    reads += ads.readADC_SingleEnded(pressure); //tekanan
    reads0 += ads.readADC_SingleEnded(arus); //arus
    i_Dis();
    i_En(rtc_addr);
    nows = rtc.now();
    i_Dis();
    i_En(I2C_ADDR);
    lcd.setCursor(3, 0);
    lcd.print(nows.year());   lcd.write('/');
    lcd2digits(nows.month()); lcd.write('/');
    lcd2digits(nows.day());
    lcd.setCursor(4, 1);
    lcd2digits(nows.hour());  lcd.write(':');
    lcd2digits(nows.minute());  lcd.write(':');
    lcd2digits(nows.second());
    off();
    power_timer0_enable();
    Narcoleptic.delay(100);
  }
  digitalWrite(BState, LOW);
  i_En(I2C_ADDR);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("VOLTAGE = "));
  lcd.setCursor(0, 1);
  lcd.print(F("CURRENT = "));

  // KONVERSI
  voltase = ((float)reads / ((float)burst * 10)) * 0.1875 / 1000.0000; // nilai voltase dari nilai DN
  tekanan = (300.00 * voltase - 150.00) * 0.01 + float(offset);
  voltase = (((float)reads0 / ((float)burst * 10)) * 0.1875);
  voltase = reads0 * 0.1875;
  if(ID=="BOGOR04"){
	ampere = ((voltase - ACSoffset) / mVperAmp) / 10000; //dalam A untuk unit 4  
  }
  else{
	ampere = ((voltase - ACSoffset) / mVperAmp) / 1000; //dalam A untuk unit 5  
  }
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);
  voltase = 5;//float(ads.readADC_SingleEnded(tegangan)) * 0.1875 / 1000.0000 * 3;
  source = "BATTERY";

  //display data to LCD
  lcd.setCursor(10, 0);
  lcd.print(voltase, 2);
  lcd.setCursor(15, 0);
  lcd.print(F("V"));
  lcd.setCursor(10, 1);
  lcd.print(ampere, 2);
  lcd.setCursor(15, 1);
  lcd.print(F("A"));
  delay(1000);
  i_En(I2C_ADDR);
  lcd.clear();
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
  delay(1000);
  i_Dis();

  //SAVE TO SD CARD
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
  spiEn();
  delay(10);
  SdFile::dateTimeCallback(dateTime);
  SD.begin(SSpin);
  i = SD.exists(str);
  // set date time callback function
  file = SD.open(str, FILE_WRITE);
  if (i == 0) {
    file.print(F("Date (YYYY-MM-DD HH:MM:SS) | "));
    file.print(F("Longitude (DD.DDDDDD°) | Latitude (DD.DDDDDD°) | Pressure (BAR)| Temperature (°C) | "));
    file.print(F("Voltage (V) | Current (AMP) | Source | ID Station | Burst interval (SECOND)| "));
    file.println(F("Operator | Server Code | Network"));
  }

  //simpan data ke SD CARD
  //FORMAT DATA = Date (YYYY-MM-DD HH:MM:SS) | Longitude (DD.DDDDDD°) | Latitude (DD.DDDDDD°) | Pressure (BAR)| Temperature (°C) |
  //              Voltage (V) | Current (AMP) | Source | ID Station | Burst interval (SECOND)|
  //        Data Interval (MINUTE) | Phone Number | Operator | Server Code | Network"));
  //TimeStamp
  file.print(tahun);  file.print('-');
  save2digits(bulan); file.print('-');
  save2digits(hari);  file.print(' ');
  save2digits(jam);   file.print(':');
  save2digits(menit); file.print(':');
  save2digits(detik); file.print('|');
  //longitude
  file.print(flon, 4);
  file.print('|');
  //latitude
  file.print(flat, 4);
  file.print('|');
  //pressure
  file.print(tekanan, 2);
  file.print('|');
  //temperature
  file.print(suhu, 2);
  file.print('|');
  //voltage
  file.print(voltase, 2);
  file.print('|');
  //current
  file.print(ampere, 4);
  file.print('|');
  //source
  file.print(source);
  file.print('|');
  //id station
  file.print(ID);
  file.print('|');
  //burst
  file.print(burst);
  file.print('|');
  //interval
  file.print(interval);
  file.print('|');
  //operator
  file.print(operators);
  file.print('|');
  //kode
  file.print(kode);
  file.print('|');
  //network
  file.print(network);  file.println('|');
  file.flush();
  file.close();
  Narcoleptic.delay(10);
  spiDis();

  //sent to server
  s_on();
  Serial.println(F("SEND DATA TO SERVER"));
  Serial.flush();
  s_off();
  server(1);

  s_on();
  s1_on();
  Serial.println(F("AT+CSCLK=2"));
  Serial1.println(F("AT+CSCLK=2"));
  Serial.flush();
  Serial1.flush();

  i_En(0x68);
  delay(10);
  nows = rtc.now();
  setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());
  i_Dis();
  s_on();
  Serial.print(nows.year());   Serial.write('/');
  Serial.print(nows.month()); Serial.print('/');
  Serial.print(nows.day());
  Serial.print(" ");
  Serial.print(nows.hour());  Serial.print(':');
  Serial.print(nows.minute());  Serial.print(':');
  Serial.println(nows.second());
  Serial.flush();
  s_off();
  off();
  while (1) {
    i_En(rtc_addr);
    nows = rtc.now();
    setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());
    i_Dis();
    start = nows.unixtime() - waktu;
    s_on();
    Serial.println(start);
    Serial.flush();
    s_off();

    if (start < ((interval * 60) - 10)) {
      Narcoleptic.delay(8000);
      power_timer0_enable();
      digitalWrite(GState, HIGH);
      delay(400);
      ledOff();
      off();
    }
    else {
      break;
    }
  }
  //bersih variabel
  bersihdata();
  on();
  i_En(rtc_addr);
  nows = rtc.now();
  setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());
  power_timer0_enable();
  digitalWrite(BState, HIGH);
  digitalWrite(GState, HIGH);
}

void ledOff() {
  digitalWrite(RState, LOW);
  digitalWrite(GState, LOW);
  digitalWrite(BState, LOW);
}

void bersihdata() {
  tahun = '0'; bulan = '0'; hari  = '0'; jam = '0'; menit = '0'; detik = '0';
  reads = '0';  reads0 = '0';  voltase = '0';  ampere = '0'; tekanan = '0'; suhu = '0';
  a = '0'; b = '0'; c = '0'; y = "";  i = '0'; result = "";
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

void s_off() {
  power_usart0_disable();
}

void s_on() {
  power_usart0_enable();
}

void s1_off() {
  power_usart1_disable();
}

void s1_on() {
  power_usart1_enable();
}

void i_En(byte alamat) {
  power_twi_enable();
  power_timer0_enable();
  Wire.begin(alamat);
}

void i_Dis() {
  power_twi_disable();
  power_timer0_disable();
}

void spiEn() {
  power_timer0_enable();
  pinMode(SSpin, OUTPUT);
  digitalWrite(SSpin, HIGH);
  power_spi_enable();
  SPCR = keep_SPCR;
  delay(100);
}

void spiDis() {
  delay(100);
  SPCR = 0;
  power_spi_disable();
  pinMode(SSpin, OUTPUT);
  digitalWrite(SSpin, LOW);
  Narcoleptic.delay(100);
}

void off() {
  power_adc_disable(); // ADC converter
  power_spi_disable(); // SPI
  power_usart0_disable(); // Serial (USART)
  power_usart1_disable();
  power_usart2_disable();
  power_timer0_disable();// Timer 0   >> millis() will stop working
  power_timer1_disable();// Timer 1   >> analogWrite() will stop working.
  power_timer2_disable();// Timer 2   >> tone() will stop working.
  power_timer3_disable();// Timer 3
  power_timer4_disable();// Timer 4
  power_timer5_disable();// Timer 5
  power_twi_disable(); // TWI (I2C)
}

void on() {
  power_spi_enable(); // SPI
  power_usart0_enable(); // Serial (USART)
  power_usart1_enable();
  power_usart2_enable();
  power_timer0_enable();// Timer 0   >> millis() will stop working
  power_timer1_enable();// Timer 1   >> analogWrite() will stop working.
  power_twi_enable(); // TWI (I2C)
}

byte ConnectAT(String cmd, int d) {
  i = 0;
  s_on();
  s1_on();
  power_timer0_enable();
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

  i_En(I2C_ADDR);
  if (i == 8) {
    lcd.setCursor(0, 1);
    lcd.print(F("                "));
    lcd.setCursor(0, 1);
    lcd.print(F("GSM MODULE OK!!"));
    Serial.println(F("OK"));
    Serial.flush();
    Serial1.flush();
    result = "OK";
    delay(750);
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print(F("                "));
    lcd.setCursor(0, 1);
    lcd.print(F("GSM MODULE ERROR"));
    Serial.println(F("SIM800L ERROR"));
    Serial.flush();
    Serial1.flush();
    off();
    ledOff();
    while (1) {
      digitalWrite(RState, HIGH);
      Narcoleptic.delay(500);
      digitalWrite(RState, LOW);
      digitalWrite(GState, HIGH);
      Narcoleptic.delay(500);
      digitalWrite(GState, LOW);

    }
  }
  return i;
}

void ceksim() {
  c = 0;
cops:
  filename = "";
  s1_on();
  s_on();
  digitalWrite(GState, HIGH);
  power_timer0_enable();
  Serial.println(F("AT+COPS?"));
  Serial1.println(F("AT+COPS?"));
  delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+COPS:")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
      }
    }
  }
  Serial.flush();
  Serial1.flush();
  off();

  a = filename.indexOf('"');
  b = filename.indexOf('"', a + 1);
  y = filename.substring(a + 1, b);
  if (y == "51089") y = "THREE";

  operators = y;
  //option if not register at network
  if (operators == "")  {
    c++;
    if (c == 9) {
      i_En(I2C_ADDR);
      lcd.clear();
      lcd.print(F("NO OPERATOR"));
      lcd.setCursor(0, 1);
      lcd.print(F("FOUND"));
      off();
      ledOff();
      while (1) {
        digitalWrite(RState, HIGH);
        power_timer0_enable();
        delay(1000);
        digitalWrite(RState, LOW);
        delay(1000);
      }
    }
    goto cops;
  }

  s_on();
  i_En(I2C_ADDR);
  Serial.print(F("OPERATOR="));
  lcd.print(operators);
  Serial.println(operators);
  y = "";
  Serial.flush();
  Serial1.flush();
  digitalWrite(GState, LOW);
  off();
}

void sinyal() {
signal:
  filename = "";
  s_on();
  s1_on();
  digitalWrite(GState, HIGH);
  power_timer0_enable();
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
  Serial1.flush();
  delay(500);

  a = (filename.substring(0, filename.indexOf(','))).toInt();
  Serial.print(a);
  Serial.print(" ");
  i_En(I2C_ADDR);
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
  digitalWrite(GState, LOW);
  off();
}

void sim800l() { //udah fix
  i_En(I2C_ADDR);
  lcd.clear();
  lcd.print(F("CHECK GSM MODULE"));
  s_on();
  Serial.println(F("CHECK SIM800L"));
  Serial.flush();
  Serial1.flush();
  off();
  power_timer0_enable();
  delay(500);

  s_on();
  s1_on();
  //Serial.println(F("AT+CSCLK=0"));
  Serial1.println(F("AT+CSCLK=0"));
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  off();
  power_timer0_enable();
  delay(500);

  i_En(I2C_ADDR);
  s_on();
  s1_on();
  lcd.setCursor(0, 1);
  for (a = 0; a < 6; a++) {
    b = ConnectAT(F("AT"), 100);
    if (b == 8) break;
  }
  Serial.flush();
  Serial1.flush();
  off();
  Narcoleptic.delay(500);

  i_En(I2C_ADDR);
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("OPS="));
  off();
  ceksim();
  power_timer0_enable();
  delay(1000);

  i_En(I2C_ADDR);
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("SIGNAL="));
  off();
  sinyal();
  power_timer0_enable();
  delay(1000);

  //SIM800L sleep mode
  s1_on();
  Serial1.println(F("AT+CSCLK=2"));
  Serial1.flush();
  off();
}

void configs() {
  a = 0;
  s_on();
  Serial.println(F("configs"));
  Serial.flush();
  s_off();
  spiEn();
  delay(100);
  i_En(I2C_ADDR);
  s_on();
  SD.begin(SSpin);
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
    Serial.flush();
    off();
    ledOff();
    while (1) {
      digitalWrite(RState, HIGH); //YELLOW RED
      digitalWrite(GState, HIGH);
      Narcoleptic.delay(1000);
      digitalWrite(GState, LOW);
      Narcoleptic.delay(1000);
    }
  }
  file.flush();
  file.close();
  delay(10);
  spiDis();

  filename = String(sdcard);
  s_on();
  Serial.println(filename);
  Serial.flush();
  s_off();

  for ( a = 0; a < sizeof(sdcard); a++) {
    sdcard[a] = (char)0;
  }
  i_En(I2C_ADDR);
  lcd.print(F("FINISH..."));
  a = filename.indexOf("\r");
  interval = filename.substring(0, a).toInt();
  b = filename.indexOf("\r", a + 1);
  burst = filename.substring(a + 1, b).toInt();
  a = filename.indexOf("\r", b + 1);
  filename = '0';
  off();
  Narcoleptic.delay(1000);
}

void bacaserial(int wait) {
  power_timer0_enable();
  s_on();
  s1_on();
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
  s_on();
  s1_on();
  power_timer0_enable();
  Serial.print(F("AT+CGATT? "));
  Serial1.println(F("AT+CGATT?"));
  Serial.flush();
  Serial1.flush();
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
  Serial.flush();
  Serial1.flush();
  if (filename.toInt() == 1) {
    //kirim data ke server
    s_on();
    s1_on();
    Serial1.println(F("AT+CIPSHUT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();

    //ATUR APN SESUAI DENGAN PROVIDER
    s_on();
    s1_on();
    apn(operators);

    //CONNECTION TYPE
    Serial1.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(100);

    //APN ID
    s_on();
    s1_on();
    result = "AT+SAPBR=3,1,\"APN\",\"" + APN + "\"";
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(100);

    //APN USER
    s_on();
    s1_on();
    result = "AT+SAPBR=3,1,\"USER\",\"" + USER + "\"";
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(100);

    //APN PASSWORD
    s_on();
    s1_on();
    result = "AT+SAPBR=3,1,\"PWD\",\"" + PWD + "\"";
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(100);

    //OPEN BEARER
    s_on();
    s1_on();
    Serial1.println(F("AT+SAPBR=1,1"));
    bacaserial(1000);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);

    //QUERY BEARER
    s_on();
    s1_on();
    Serial.println(F("AT+SAPBR=2,1"));
    Serial1.println(F("AT+SAPBR=2,1"));
    power_timer0_enable();
    start = millis();
    while (start + 2000 > millis()) {
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
    Narcoleptic.delay(500);

    //TERMINATE HTTP SERVICE
    s_on();
    s1_on();
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    off();
    Narcoleptic.delay(100);
    s_on();
    s1_on();
    Serial1.println(F("AT+HTTPINIT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);

    //SET HTTP PARAMETERS VALUE
    s_on();
    s1_on();
    Serial1.println(F("AT+HTTPPARA=\"CID\",1"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);
    if (t == 1) { // send data measurement to server
      //SET HTTP URL
      s_on();
      s1_on();
      Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      bacaserial(1000);
      Serial.flush();
      Serial1.flush();
      off();
      //http://www.mantisid.id/api/product/pdam_dt_c.php?="Data":"'2017-11-05 10:00:00', '111.111111', '-6.2222222', '400.33', '34.00', '5.05', '1.66', 'pdam_001', '5', '3'"
      //"Data":"'2018-02-13 20:30:00','111.111111','-6.2222222','400.33','34.00','5.05','1.6645','BOGOR05','5','3'"
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
      y += String(ampere, 4) + "','";
      y += String(source) + "','";
      y += String(ID) + "','";
      y += String(burst) + "','";
      y += String(interval) + "'\"}";
    }
    if (t == 2) { // send data kuota to server
      //GET TIME
      i_En(rtc_addr);
      nows = rtc.now();
      i_Dis();
      tahun = nows.year();
      bulan = nows.month();
      hari = nows.day();
      jam = nows.hour();
      menit = nows.minute();
      detik = nows.second();

      //SET HTTP URL
      s1_on();
      s_on();
      //Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_sim_c.php\""));
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
      spiEn();
      delay(10);
      SdFile::dateTimeCallback(dateTime);
      delay(10);
      SD.begin(SSpin);
      file = SD.open(str, FILE_WRITE);
      file.println(y);
      Serial.println(y);
      Serial.flush();
      file.flush();
      file.close();
      spiDis();
      Narcoleptic.delay(10);
    }
    //SET HTTP DATA FOR SENDING TO SERVER
    s_on();
    s1_on();
    power_timer0_enable();
    result = "AT+HTTPDATA=" + String(y.length() + 1) + ",15000";
    Serial.println(result);
    Serial1.println(result);
    Serial.flush();
    Serial1.flush();
    while (Serial1.available() > 0) {
      while (Serial1.find("DOWNLOAD") == false) {
      }
    }

    //SEND DATA
    s_on();
    s1_on();
    Serial.println(y);
    Serial1.println(y);
    Serial.flush();
    Serial1.flush();
    bacaserial(1000);

    //HTTP METHOD ACTION
    filename = "";
    power_timer0_enable();
    s_on();
    s1_on();
    start = millis();
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
    while ((start + 30000) > millis()) {
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
    Narcoleptic.delay(500);
    s_on();
    s1_on();
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(200);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);
    s_on();
    s1_on();
    Serial.println(F("AT+SAPBR=0,1"));
    Serial1.println(F("AT+SAPBR=0,1"));
    Serial.flush();
    Serial1.flush();
    while (start + 1000 > millis()) {
      while (Serial1.available() > 0) {
        if (Serial1.find("OK")) {
          a = 1;
          break;
        }
      }
      if (a = 1) break;
    }
    a = '0';
    a = filename.indexOf(',');
    b = filename.indexOf(',', a + 1);
    kode = filename.substring(a + 1, b).toInt();
    statuscode(kode);
  }
  else {
    network = "Error";
    kode = 999;
    s_on();
    Serial.println(F("NETWORK ERROR"));
    Serial.flush();
    s_off();
  }
}

void cekkuota() {
  c = 0;
  i_En(I2C_ADDR);
  lcd.clear();
  lcd.print(F("CHECK BALANCE"));
  lcd.setCursor(0, 1);
  lcd.print(F("Init...."));
  s_on();
  s1_on();
  Serial1.println("AT");
  Serial.flush();
  Serial1.flush();
  bacaserial(100);
  off();
  Narcoleptic.delay(200);
  s_on();
  s1_on();
  Serial1.println("AT+CSCLK=0");
  Serial.flush();
  Serial1.flush();
  off();
  Narcoleptic.delay(200);
  s_on();
  s1_on();
  Serial1.println("AT+CSCLK=0");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  off();
  i_En(I2C_ADDR);
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("Init Finish!"));
  Narcoleptic.delay(500);
top:
  if (c == 5)  {
    sms = "sisa pulsa tidak diketahui";
    kuota = "sisa kuota tidak diketahui";
    goto down;
  }
  sms = "";
  kuota = "";

  power_timer0_enable();
  s_on();
  s1_on();
  Serial1.println("AT+CUSD=2");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  off();
  Narcoleptic.delay(200);

  s_on();
  s1_on();
  Serial1.println("AT+CUSD=1");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  off();
  Narcoleptic.delay(400);
  sms = "";
  power_timer0_enable();
  s_on();
  s1_on();
  start = millis();
  i_En(I2C_ADDR);
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("*888#"));
  Serial1.println("AT+CUSD=1,\"*888#\"");
  Serial1.flush();

  while ((start + 10000) > millis()) {
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
  c = sms.indexOf('/', a);
  kuota = sms.substring(c - 11, c + 8);
  Serial.println(kuota);
  sms = sms.substring(a + 5, b);
  sms = sms + '.' + kuota;
  Serial.println(sms);
  Serial.flush();

  Narcoleptic.delay(100);
  start = millis();
  i_En(I2C_ADDR);
  lcd.print(F("3"));
  Serial1.println("AT+CUSD=1,\"3\"");
  while ((start + 5000) > millis()) {
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
  Narcoleptic.delay(100);

  kuota = "";
  start = millis();
  i_En(I2C_ADDR);
  lcd.print(F("#"));
  Narcoleptic.delay(300);
  lcd.print(F("2"));
  Serial1.println("AT+CUSD=1,\"2\"");
  while ((start + 5000) > millis()) {
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
  a = kuota.indexOf(':', a + 1);
  kuota = kuota.substring(a + 2, b);

down:
  a = 0; b = 0; i = 0;
  Serial.flush();
  Serial1.flush();
  Serial1.println("AT+CUSD=2");
  bacaserial(200);
  Serial.flush();
  Serial1.flush();
  Serial.println(sms);
  Serial.print(F("KUOTA : "));
  Serial.println(kuota);
  Serial.flush();
  Serial1.flush();
  server(2); //send kuota to server
  i_En(I2C_ADDR);
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("Finish!!!"));
  s_on();
  Serial.println(F("cek kuota selesai"));
  Serial.flush();
  s_off();
}

// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {
  i_En(rtc_addr);
  nows = rtc.now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(nows.year(), nows.month(), nows.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(nows.hour(), nows.minute(), nows.second());
}

