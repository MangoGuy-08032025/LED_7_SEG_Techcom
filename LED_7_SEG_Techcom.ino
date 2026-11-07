#include <Arduino.h>
#include "ota_update.h"
#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <time.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>

#define EEPROM_ADDR 0x57

Preferences prefs;
WebServer server(80);

RTC_DS3231 rtc;

#define DEFAULT_WIFI_OTA    "Linh Anh"
#define DEFAULT_PASS_OTA    "08032025"

// DISPLAY 1 (10 digit)
//
#define DISP1_DATA 0
#define DISP1_CLK  4
#define DISP1_LAT  5

#define CLEAR_PIN 16
#define UPDATE_PIN 15

#define CAM_BIEN_S5 32
#define CHON_LED_SS 33
#define KICH_HOAT_SS 34

#define KHONG_CO_HANG 0
#define DANG_CO_HANG 1
#define HANG_VUA_DI_QUA 2
//
// DISPLAY 2 (17 digit)
//
#define DISP2_DATA 27
#define DISP2_CLK  25
#define DISP2_LAT  26

#define DISPLAY1_DIGITS 10
#define DISPLAY2_DIGITS 16

#define DEN_DO 17
#define DEN_VANG 18
#define DEN_XANH 19

//====================================================
// Character -> 7 Segment
// bit:
// DP G F E D C B A
//====================================================
int trang_thai_cam_bien = KHONG_CO_HANG;

int muc_tieu_hien_tai = 12;
int san_luong_hien_tai = 0;
int hieu_suat = 10;
int setup_mode = 0;
int so_hang_trong_moi_hop = 4;
String Wifi = "SMC";
String Password = "Techcom2025@";
String Line = "1";
String Ten_Line = "Line " + Line; 
String Model = "DVC2834";
int muc_tieu = 1000;
int gio_bat_dau = 8, phut_bat_dau = 0, gio_giai_lao_0 = 12, phut_giai_lao_0 = 0, thoi_gian_giai_lao_0 = 60, gio_giai_lao_1 = 16, phut_giai_lao_1 = 0, thoi_gian_giai_lao_1 = 60;
int gio_ket_thuc = 0, phut_ket_thuc = 0;
int so_phut_lam_viec_thuc_te = 0;
int so_phut_hien_tai = 0;
int so_phut_da_nghi_giai_lao_0 = 0, so_phut_da_nghi_giai_lao_1 = 0;
int gio_hien_tai = 0, phut_hien_tai = 0, giay_hien_tai = 0, ngay_hien_tai = 0, thang_hien_tai = 0, nam_hien_tai = 0;
int thoi_gian_bat_dau_theo_phut = 0, thoi_gian_bat_dau_giai_lao_0_theo_phut = 0, thoi_gian_ket_thuc_giai_lao_0_theo_phut = 0;
int thoi_gian_bat_dau_giai_lao_1_theo_phut = 0, thoi_gian_ket_thuc_giai_lao_1_theo_phut = 0, thoi_gian_ket_thuc_theo_phut = 0;
int thoi_gian_lam_thuc_te_theo_phut = 0, thoi_gian_giai_lao_0_thuc_te_theo_phut = 0, thoi_gian_giai_lao_1_thuc_te_theo_phut = 0;
int thoi_gian_hien_tai_theo_phut = 0;
int thoi_gian_mot_ca_lam_viec_theo_phut = 0;
int trang_thai_display_1 = 0;

void loadConfig()
{
  prefs.begin("cfg", true);

  Line = prefs.getString("Line", "1");
  Wifi = prefs.getString("Wifi", "SMC");
  Password = prefs.getString("Password", "Techcom2025@");
  Model = prefs.getString("Model", "DVC2834");

  muc_tieu = prefs.getInt("Target", 10);

  so_hang_trong_moi_hop = prefs.getInt("SHTH", 3);

  gio_bat_dau = prefs.getInt("GBD", 8);
  phut_bat_dau = prefs.getInt("PBD", 0);

  gio_giai_lao_0 = prefs.getInt("GGL0", 12);
  phut_giai_lao_0 = prefs.getInt("PGL0", 0);
  thoi_gian_giai_lao_0 = prefs.getInt("TGL0", 60);

  gio_giai_lao_1 = prefs.getInt("GGL1", 16);
  phut_giai_lao_1 = prefs.getInt("PGL1", 0);
  thoi_gian_giai_lao_1 = prefs.getInt("TGL1", 60);

  gio_ket_thuc = prefs.getInt("GKT", 17);
  phut_ket_thuc = prefs.getInt("PKT", 0);

  prefs.end();
}

//================================================
// GHI FLASH
//================================================

void saveConfig()
{
  prefs.begin("cfg", false);

  prefs.putString("Line", Line);
  prefs.putString("Wifi", Wifi);
  prefs.putString("Password", Password);
  prefs.putString("Model", Model);

  prefs.putInt("Target", muc_tieu);

  prefs.putInt("SHTH", so_hang_trong_moi_hop);

  prefs.putInt("GBD", gio_bat_dau);
  prefs.putInt("PBD", phut_bat_dau);

  prefs.putInt("GGL0", gio_giai_lao_0);
  prefs.putInt("PGL0", phut_giai_lao_0);
  prefs.putInt("TGL0", thoi_gian_giai_lao_0);

  prefs.putInt("GGL1", gio_giai_lao_1);
  prefs.putInt("PGL1", phut_giai_lao_1);
  prefs.putInt("TGL1", thoi_gian_giai_lao_1);

  prefs.putInt("GKT", gio_ket_thuc);
  prefs.putInt("PKT", phut_ket_thuc);

  prefs.end();

  writeSanLuong(san_luong_hien_tai);
}

//================================================
// HTML
//================================================

String makePage()
{
  String html;

  html += R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">

<style>

body{
font-family:Arial;
max-width:750px;
margin:auto;
padding:10px;
background:#f0f0f0;
}

.card{
background:white;
padding:15px;
border-radius:10px;
box-shadow:0 0 5px #999;
}

h2{
text-align:center;
}

.row{
display:flex;
margin-bottom:10px;
gap:10px;
}

.col{
flex:1;
}

input{
width:100%;
padding:8px;
font-size:18px;
box-sizing:border-box;
}

button{
width:100%;
padding:12px;
font-size:20px;
background:#2196F3;
border:none;
color:white;
border-radius:5px;
}

label{
font-weight:bold;
}

</style>
</head>
<body>

<div class='card'>

<h2>TECHCOM CONFIG</h2>

<form action='/save'>

<label>LINE</label>
<input name='Line' value='
)rawliteral";

  html += Line;

  html += R"rawliteral('>

<br><br>

<label>Wifi</label>
<input name='Wifi' value='
)rawliteral";

  html += Wifi;

  html += R"rawliteral('>

<br><br>

<label>Password</label>
<input name='Password' value='
)rawliteral";

  html += Password;

  html += R"rawliteral('>

<br><br>

<label>MODEL</label>
<input name='Model' value='
)rawliteral";

  html += Model;

  html += R"rawliteral('>

<br><br>

<label>MUC TIEU</label>
<input type='number' name='muc_tieu' value=')rawliteral";

  html += String(muc_tieu);

  html += R"rawliteral('>

<br><br>

<label>SAN_LUONG_HIEN_TAI</label>
<input type='number' name='san_luong_hien_tai' value=')rawliteral";

  html += String(san_luong_hien_tai);

  html += R"rawliteral('>

<br><br>

<label>SO_HANG_TRONG_MOI_HOP</label>
<input type='number' name='so_hang_trong_moi_hop' value=')rawliteral";

  html += String(so_hang_trong_moi_hop);

  html += R"rawliteral('>

<br><br>

<h3>THOI GIAN HIEN TAI</h3>

<div class='row'>
<div class='col'>
<label>Gio</label>
<input type='number' min='0' max='23'
name='gio_hien_tai' value=')rawliteral";

  html += String(gio_hien_tai);

  html += R"rawliteral('>
</div>

<div class='col'>
<label>Phut</label>
<input type='number' min='0' max='59'
name='phut_hien_tai' value=')rawliteral";

  html += String(phut_hien_tai);

  html += R"rawliteral('>
</div>

<div class='col'>
<label>Giay</label>
<input type='number' min='0' max='59'
name='giay_hien_tai' value=')rawliteral";

  html += String(giay_hien_tai);

  html += R"rawliteral('>
</div>

</div>

<h3>NGAY HIEN TAI</h3>

<div class='row'>
<div class='col'>
<label>Gio</label>
<input type='number' min='1' max='31'
name='ngay_hien_tai' value=')rawliteral";

  html += String(ngay_hien_tai);

  html += R"rawliteral('>
</div>

<div class='col'>
<label>Thang</label>
<input type='number' min='1' max='12'
name='thang_hien_tai' value=')rawliteral";

  html += String(thang_hien_tai);

  html += R"rawliteral('>
</div>

<div class='col'>
<label>Nam</label>
<input type='number' min='2000' max='2100'
name='nam_hien_tai' value=')rawliteral";

  html += String(nam_hien_tai);

  html += R"rawliteral('>
</div>

</div>

<h3>BAT DAU</h3>

<div class='row'>
<div class='col'>
<label>Gio</label>
<input type='number' min='0' max='23'
name='gio_bd' value=')rawliteral";

  html += String(gio_bat_dau);

  html += R"rawliteral('>
</div>

<div class='col'>
<label>Phut</label>
<input type='number' min='0' max='59'
name='phut_bd' value=')rawliteral";

  html += String(phut_bat_dau);

  html += R"rawliteral('>
</div>
</div>

<h3>GIAI LAO 0</h3>

<div class='row'>
<div class='col'>
<input type='number'
name='gio_gl0' value=')rawliteral";

  html += String(gio_giai_lao_0);

  html += R"rawliteral('>
</div>

<div class='col'>
<input type='number'
name='phut_gl0' value=')rawliteral";

  html += String(phut_giai_lao_0);

  html += R"rawliteral('>
</div>

<div class='col'>
<input type='number'
name='tg_gl0' value=')rawliteral";

  html += String(thoi_gian_giai_lao_0);

  html += R"rawliteral('>
</div>
</div>

<h3>GIAI LAO 1</h3>

<div class='row'>
<div class='col'>
<input type='number'
name='gio_gl1' value=')rawliteral";

  html += String(gio_giai_lao_1);

  html += R"rawliteral('>
</div>

<div class='col'>
<input type='number'
name='phut_gl1' value=')rawliteral";

  html += String(phut_giai_lao_1);

  html += R"rawliteral('>
</div>

<div class='col'>
<input type='number'
name='tg_gl1' value=')rawliteral";

  html += String(thoi_gian_giai_lao_1);

  html += R"rawliteral('>
</div>
</div>

<h3>KET THUC</h3>

<div class='row'>

<div class='col'>
<input type='number'
name='gio_kt' value=')rawliteral";

  html += String(gio_ket_thuc);

  html += R"rawliteral('>
</div>

<div class='col'>
<input type='number'
name='phut_kt' value=')rawliteral";

  html += String(phut_ket_thuc);

  html += R"rawliteral('>
</div>

</div>

<br>

<button type='submit'>
SAVE CONFIG
</button>

</form>

</div>

</body>
</html>
)rawliteral";

  return html;
}

//================================================
// SAVE
//================================================

void handleSave()
{
  Line = server.arg("Line");
  Model = server.arg("Model");
  Wifi = server.arg("Wifi");
  Password = server.arg("Password");
  san_luong_hien_tai = server.arg("san_luong_hien_tai").toInt();
  muc_tieu = server.arg("muc_tieu").toInt();
  so_hang_trong_moi_hop = server.arg("so_hang_trong_moi_hop").toInt();
  gio_bat_dau = server.arg("gio_bd").toInt();
  phut_bat_dau = server.arg("phut_bd").toInt();

  gio_giai_lao_0 = server.arg("gio_gl0").toInt();
  phut_giai_lao_0 = server.arg("phut_gl0").toInt();
  thoi_gian_giai_lao_0 = server.arg("tg_gl0").toInt();

  gio_giai_lao_1 = server.arg("gio_gl1").toInt();
  phut_giai_lao_1 = server.arg("phut_gl1").toInt();
  thoi_gian_giai_lao_1 = server.arg("tg_gl1").toInt();

  gio_ket_thuc = server.arg("gio_kt").toInt();
  phut_ket_thuc = server.arg("phut_kt").toInt();

  saveConfig();

  String page;

  page += "<h2>Saved Successfully</h2>";
  page += "<a href='/'>Back</a>";

  server.send(200, "text/html", page);
  delay(2000);
  ngay_hien_tai = server.arg("ngay_hien_tai").toInt();
  thang_hien_tai = server.arg("thang_hien_tai").toInt();
  nam_hien_tai = server.arg("nam_hien_tai").toInt();
  gio_hien_tai = server.arg("gio_hien_tai").toInt();
  phut_hien_tai = server.arg("phut_hien_tai").toInt();
  giay_hien_tai = server.arg("giay_hien_tai").toInt();
  DateTime ntpTime(
        nam_hien_tai,
        thang_hien_tai,
        ngay_hien_tai,
        gio_hien_tai,
        phut_hien_tai,
        giay_hien_tai
    );
  rtc.adjust(ntpTime);
  delay(1000);
  ESP.restart();
}


void getRTC()
{
    DateTime now = rtc.now();
    ngay_hien_tai = now.day();
    thang_hien_tai = now.month();
    nam_hien_tai = now.year();
    gio_hien_tai = now.hour();
    phut_hien_tai = now.minute();
    giay_hien_tai = now.second();
}

bool syncRTCfromInternet()
{
    Serial.println("Connecting WiFi...");

    WiFi.begin(DEFAULT_WIFI_OTA, DEFAULT_PASS_OTA);

    uint32_t timeout = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if (millis() - timeout > 15000)
        {
            Serial.println("\nWiFi Failed");
            return false;
        }
    }

    Serial.println("\nWiFi Connected");

    configTime(
        7 * 3600,    // GMT+7
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );
    struct tm timeinfo;

    timeout = millis();

    while (!getLocalTime(&timeinfo))
    {
        Serial.println("Step 3");
        delay(500);
        Serial.print("*");

        if (millis() - timeout > 100000)
        {
            Serial.println("\nNTP Failed");
            return false;
        }
    }
    DateTime ntpTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );
    rtc.adjust(ntpTime);

    Serial.println("\nRTC Updated");

    getRTC();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    return true;
}

uint8_t charTo7Seg_LED_TO(char c)
{
    uint8_t seg;

    switch (toupper(c))
    {
        case '0': seg = 0x3F; break;
        case '1': seg = 0x06; break;
        case '2': seg = 0x5B; break;
        case '3': seg = 0x4F; break;
        case '4': seg = 0x66; break;
        case '5': seg = 0x6D; break;
        case '6': seg = 0x7D; break;
        case '7': seg = 0x07; break;
        case '8': seg = 0x7F; break;
        case '9': seg = 0x6F; break;

        case 'A': seg = 0x77; break;
        case 'B': seg = 0x7C; break;
        case 'C': seg = 0x39; break;
        case 'D': seg = 0x5E; break;
        case 'E': seg = 0x79; break;
        case 'F': seg = 0x71; break;
        case 'V': seg = 0x3E; break;
        case 'P': seg = 0x73; break;
        case 'T': seg = 0x33; break;
        case 'L': seg = 0x38; break;
        case 'S': seg = 0x6D; break;
        case '-': seg = 0x40; break;
        case ' ': seg = 0x00; break;

        default : seg = 0x00; break;
    }

    return seg;    // Đảo bit cho Common Anode
}

uint8_t charTo7Seg(char c)
{
    uint8_t seg;

    switch (toupper(c))
    {
        case '0': seg = 0x3F; break;
        case '1': seg = 0x06; break;
        case '2': seg = 0x5B; break;
        case '3': seg = 0x4F; break;
        case '4': seg = 0x66; break;
        case '5': seg = 0x6D; break;
        case '6': seg = 0x7D; break;
        case '7': seg = 0x07; break;
        case '8': seg = 0x7F; break;
        case '9': seg = 0x6F; break;

        case 'A': seg = 0x77; break;
        case 'B': seg = 0x7C; break;
        case 'C': seg = 0x39; break;
        case 'D': seg = 0x5E; break;
        case 'E': seg = 0x79; break;
        case 'F': seg = 0x71; break;
        case 'V': seg = 0x3E; break;
        case 'P': seg = 0x73; break;
        case 'T': seg = 0x33; break;
        case 'L': seg = 0x38; break;
        case 'S': seg = 0x6D; break;
        case '-': seg = 0x40; break;
        case ' ': seg = 0x00; break;

        default : seg = 0x00; break;
    }

    return ~seg;    // Đảo bit cho Common Anode
}


//====================================================
// ShiftOut
//====================================================

void shiftOutByte(
    uint8_t dataPin,
    uint8_t clkPin,
    uint8_t value)
{
    for (int i = 7; i >= 0; i--)
    {
        digitalWrite(clkPin, LOW);

        digitalWrite(
            dataPin,
            (value >> i) & 0x01);

        digitalWrite(clkPin, HIGH);
    }
}


//====================================================
// Build Segment Buffer
// Hỗ trợ dấu chấm thập phân
//====================================================

int buildBuffer(
    const String& text,
    uint8_t* buffer,
    int maxLen)
{
    int count = 0;

    for (int i = 0; i < text.length(); i++)
    {
        char c = text[i];

        if (c == '.')
        {
            if (count > 0)
            {
                buffer[count - 1] |= 0x80;
            }
            continue;
        }

        if (count >= maxLen)
            break;

        buffer[count++] = charTo7Seg(c);
    }

    return count;
}

int buildBuffer_LED_TO(
    const String& text,
    uint8_t* buffer,
    int maxLen)
{
    int count = 0;

    for (int i = 0; i < text.length(); i++)
    {
        char c = text[i];

        if (c == '.')
        {
            if (count > 0)
            {
                buffer[count - 1] |= 0x80;
            }
            continue;
        }

        if (count >= maxLen)
            break;

        buffer[count++] = charTo7Seg_LED_TO(c);
    }

    return count;
}

//====================================================
// Generic Display Function
//====================================================

void displayString(
    const String& text,
    uint8_t digits,
    uint8_t dataPin,
    uint8_t clkPin,
    uint8_t latPin)
{
    uint8_t segBuffer[32];

    for (int i = 0; i < 32; i++)
        segBuffer[i] = 0;

    
    int used = buildBuffer(
        text,
        segBuffer,
        digits);

    digitalWrite(latPin, LOW);

    for (int d = digits - 1; d >= 0; d--)
    {
        uint8_t seg = 0;

        if (d < used)
            seg = segBuffer[d];

        shiftOutByte(
            dataPin,
            clkPin,
            seg);
    }

    digitalWrite(latPin, HIGH);
}

void displayString_LED_TO(
    const String& text,
    uint8_t digits,
    uint8_t dataPin,
    uint8_t clkPin,
    uint8_t latPin)
{
    uint8_t segBuffer[32];

    for (int i = 0; i < 32; i++)
        segBuffer[i] = 0;

    int used = buildBuffer_LED_TO(
        text,
        segBuffer,
        digits);

    digitalWrite(latPin, LOW);

    for (int d = digits - 1; d >= 0; d--)
    {
        uint8_t seg = 0;

        if (d < used)
            seg = segBuffer[d];

        shiftOutByte(
            dataPin,
            clkPin,
            seg);
    }

    digitalWrite(latPin, HIGH);
}

//====================================================
// Display 1
//====================================================

void display1(const String& text)
{
  if(digitalRead(CHON_LED_SS) == HIGH)
  {
        displayString(
        text,
        DISPLAY1_DIGITS,
        DISP1_DATA,
        DISP1_CLK,
        DISP1_LAT);
  }
  else
  {
        displayString_LED_TO(
        text,
        DISPLAY1_DIGITS,
        DISP1_DATA,
        DISP1_CLK,
        DISP1_LAT);
  }
}


//====================================================
// Display 2
//====================================================

void display2(const String& text)
{
  if(digitalRead(CHON_LED_SS) == HIGH)
  {
        displayString(
        text,
        DISPLAY2_DIGITS,
        DISP2_DATA,
        DISP2_CLK,
        DISP2_LAT);
  }
  else
  {
        displayString_LED_TO(
        text,
        DISPLAY2_DIGITS,
        DISP2_DATA,
        DISP2_CLK,
        DISP2_LAT);
  }

}
String to4DigitString(int value)
{
    value = constrain(value, 0, 9999);

    return String(value / 1000) +
           String((value / 100) % 10) +
           String((value / 10) % 10) +
           String(value % 10);
}

String to3DigitString(int value)
{
    value = constrain(value, 0, 999);

    return String((value / 100) % 10) +
           String(value / 10) +
           String(value % 10);
}


//=====================
// Ghi 1 số int 16-bit
//=====================
void writeSanLuong(int value)
{
    Wire.beginTransmission(EEPROM_ADDR);

    // Địa chỉ bộ nhớ EEPROM
    Wire.write(0x00); // High byte address
    Wire.write(0x00); // Low byte address

    // Dữ liệu
    Wire.write((value >> 8) & 0xFF);
    Wire.write(value & 0xFF);

    Wire.endTransmission();

    delay(10); // Chờ EEPROM ghi xong
}

//=====================
// Đọc 1 số int 16-bit
//=====================
int readSanLuong()
{
    Wire.beginTransmission(EEPROM_ADDR);

    Wire.write(0x00); // High byte address
    Wire.write(0x00); // Low byte address

    Wire.endTransmission();

    Wire.requestFrom(EEPROM_ADDR, 2);

    if (Wire.available() >= 2)
    {
        byte highByte = Wire.read();
        byte lowByte  = Wire.read();

        return (highByte << 8) | lowByte;
    }

    return 0;
}

//====================================================
// Setup
//====================================================

void setup()
{
    Serial.begin(115200);
    pinMode(DISP1_DATA, OUTPUT);
    pinMode(DISP1_CLK, OUTPUT);
    pinMode(DISP1_LAT, OUTPUT);

    pinMode(DISP2_DATA, OUTPUT);
    pinMode(DISP2_CLK, OUTPUT);
    pinMode(DISP2_LAT, OUTPUT);

    pinMode(DEN_DO, OUTPUT);
    pinMode(DEN_VANG, OUTPUT);
    pinMode(DEN_XANH, OUTPUT);

    digitalWrite(DISP1_DATA, HIGH);
    digitalWrite(DISP1_CLK, HIGH);
    digitalWrite(DISP1_LAT, HIGH);
    digitalWrite(DISP2_DATA, HIGH);
    digitalWrite(DISP2_CLK, HIGH);
    digitalWrite(DISP2_LAT, HIGH);


    pinMode(UPDATE_PIN, INPUT_PULLUP);
    pinMode(CLEAR_PIN, INPUT_PULLUP);
    pinMode(CAM_BIEN_S5, INPUT_PULLUP);
    pinMode(CHON_LED_SS, INPUT_PULLUP);
    pinMode(KICH_HOAT_SS, INPUT_PULLUP);
    if ((digitalRead(UPDATE_PIN) == LOW) )
    {
      WiFi.mode(WIFI_STA);  // Chế độ thu wifi
      WiFi.begin(Wifi.c_str(), Password.c_str());
      while (WiFi.status() != WL_CONNECTED) 
      {
        Serial.print(".");
        delay(50);
        display1("VPDATE-C  ");
        display2("----------------");
      }
      display1(FW_VERSION);
      display2("----------------");
      checkAndUpdateFirmware();
    }
    display1("VPDATE-T  ");
    display2("----------------");
    Wire.begin(21,22);
    if (!rtc.begin())
    {
        Serial.println("Khong tim thay DS3231");
        display1("FAIL-T    ");
    }
    else
    {
        if(true == syncRTCfromInternet())
        {
          display1("SUC-T     ");
        }
        else
        {
          display1("FA1L-T    ");
        }
        
    }
    
    loadConfig();
    san_luong_hien_tai = readSanLuong();

    delay(2000);
}

//====================================================
// Loop
//====================================================

void loop()
{
  if(hieu_suat < 80)
  {
    digitalWrite(DEN_DO, HIGH);
    digitalWrite(DEN_XANH, LOW);
    digitalWrite(DEN_VANG, LOW);
  }
  if((hieu_suat > 80) && (hieu_suat < 90))
  {
    digitalWrite(DEN_DO, LOW);
    digitalWrite(DEN_XANH, LOW);
    digitalWrite(DEN_VANG, HIGH);
  }
  if(hieu_suat > 90)
  {
    digitalWrite(DEN_DO, LOW);
    digitalWrite(DEN_XANH, HIGH);
    digitalWrite(DEN_VANG, LOW);
  }

  if(digitalRead(CLEAR_PIN) == LOW)
  {
    delay(1000);
    if(digitalRead(CLEAR_PIN) == LOW)
    {
      san_luong_hien_tai = 0;
      writeSanLuong(san_luong_hien_tai);
    }

  }
  thoi_gian_hien_tai_theo_phut = gio_hien_tai*60 + phut_hien_tai;
  thoi_gian_bat_dau_theo_phut = gio_bat_dau * 60 + phut_bat_dau;
  thoi_gian_bat_dau_giai_lao_0_theo_phut = gio_giai_lao_0 * 60 + phut_giai_lao_0;
  thoi_gian_bat_dau_giai_lao_1_theo_phut = gio_giai_lao_1 * 60 + phut_giai_lao_1;
  thoi_gian_ket_thuc_theo_phut = gio_ket_thuc * 60 + phut_ket_thuc;

  thoi_gian_giai_lao_0_thuc_te_theo_phut = thoi_gian_hien_tai_theo_phut - thoi_gian_bat_dau_giai_lao_0_theo_phut;
  if (thoi_gian_giai_lao_0_thuc_te_theo_phut <= 0)
  {
    thoi_gian_giai_lao_0_thuc_te_theo_phut = 0;
  } 
  else if(thoi_gian_giai_lao_0_thuc_te_theo_phut >= thoi_gian_giai_lao_0)
  {
    thoi_gian_giai_lao_0_thuc_te_theo_phut = thoi_gian_giai_lao_0;
  }
  thoi_gian_giai_lao_1_thuc_te_theo_phut = thoi_gian_hien_tai_theo_phut - thoi_gian_bat_dau_giai_lao_1_theo_phut;
  if (thoi_gian_giai_lao_1_thuc_te_theo_phut <= 0)
  {
    thoi_gian_giai_lao_1_thuc_te_theo_phut = 0;
  } 
  else if(thoi_gian_giai_lao_1_thuc_te_theo_phut >= thoi_gian_giai_lao_1)
  {
    thoi_gian_giai_lao_1_thuc_te_theo_phut = thoi_gian_giai_lao_1;
  }

  thoi_gian_lam_thuc_te_theo_phut = thoi_gian_hien_tai_theo_phut - thoi_gian_bat_dau_theo_phut - thoi_gian_giai_lao_0_thuc_te_theo_phut - thoi_gian_giai_lao_1_thuc_te_theo_phut;
  thoi_gian_mot_ca_lam_viec_theo_phut = thoi_gian_ket_thuc_theo_phut - thoi_gian_bat_dau_theo_phut - thoi_gian_giai_lao_0 - thoi_gian_giai_lao_1 + 1;
  if(thoi_gian_lam_thuc_te_theo_phut <= 0)
  {
    thoi_gian_lam_thuc_te_theo_phut = 0;
  }
  muc_tieu_hien_tai = (((float)muc_tieu / (float)thoi_gian_mot_ca_lam_viec_theo_phut) * (float(thoi_gian_lam_thuc_te_theo_phut)));
  if(muc_tieu_hien_tai > muc_tieu)
  {
    muc_tieu_hien_tai = muc_tieu;
  }
  hieu_suat = int((float)san_luong_hien_tai / (float)muc_tieu_hien_tai * 100.0);


  getRTC();
    if (digitalRead(UPDATE_PIN) == LOW )
    {
        delay(1000);
        if(digitalRead(UPDATE_PIN) == LOW)
        {
          trang_thai_display_1 = 3;
            setup_mode = 1;
            WiFi.mode(WIFI_AP);
            WiFi.softAP(
                 Ten_Line,
                "12345678");

            Serial.println();
            Serial.print("IP: ");
            Serial.println(WiFi.softAPIP());

            server.on("/", []()
                        { server.send(200, "text/html", makePage()); });

            server.on("/save", handleSave);

            server.begin();

            Serial.println("WebServer Started");
        }
    }
    if (setup_mode == 1)
    {
        server.handleClient();
    }
  if(trang_thai_display_1 == 0)
  {
    while (Model.length() < 10)
    {
        Model = " " + Model;
    }
    display1(Model);
  }

    if(trang_thai_display_1 == 1)
    {
        String chuoi_thoi_gian = "  ";
        chuoi_thoi_gian = chuoi_thoi_gian + String(gio_hien_tai) + "-" + String(phut_hien_tai) + "-" + String(giay_hien_tai) + "   ";
        display1(chuoi_thoi_gian);
    }

    if(trang_thai_display_1 == 2)
    {
        String ngay_thang_hien_tai = "";
        ngay_thang_hien_tai = ngay_thang_hien_tai + String(ngay_hien_tai) + "-" + String(thang_hien_tai) + "-" + String(nam_hien_tai) + "   ";
        display1(ngay_thang_hien_tai);
    }

    if(trang_thai_display_1 == 3)
    {
        String config_str = "SETVP";
        display1(config_str);
    }


    if(digitalRead(CHON_LED_SS) == HIGH)
    {
      String msg = to4DigitString(muc_tieu) + to4DigitString(muc_tieu_hien_tai) + to4DigitString(san_luong_hien_tai) + to4DigitString(hieu_suat);
      display2(msg);
    }
    else
    {
      String msg_led_to = to4DigitString(muc_tieu) + to4DigitString(muc_tieu_hien_tai) + to4DigitString(san_luong_hien_tai) + to3DigitString(hieu_suat);
      display2(msg_led_to);
    }

  
  
  if((digitalRead(CAM_BIEN_S5) == LOW) && (trang_thai_cam_bien == KHONG_CO_HANG))
  {
    delay(50);
    if(digitalRead(CAM_BIEN_S5) == LOW)
    {
      trang_thai_cam_bien = DANG_CO_HANG;
    }
  }
  if ((digitalRead(CAM_BIEN_S5) == HIGH) && (trang_thai_cam_bien == DANG_CO_HANG))
  {
    delay(50);
    if(digitalRead(CAM_BIEN_S5) == HIGH)
    {
      trang_thai_cam_bien = HANG_VUA_DI_QUA;
    }    
  }
  if(trang_thai_cam_bien == HANG_VUA_DI_QUA)
  {
    san_luong_hien_tai = san_luong_hien_tai + so_hang_trong_moi_hop;
    writeSanLuong(san_luong_hien_tai);
    delay(1000);
    trang_thai_cam_bien = KHONG_CO_HANG;
    trang_thai_display_1 += 1;
    if (trang_thai_display_1 > 2)
    {
        trang_thai_display_1 = 0;
    }
  }

  



}