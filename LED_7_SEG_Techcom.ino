#include <Arduino.h>
#include "ota_update.h"
#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <time.h>
RTC_DS3231 rtc;

#define DEFAULT_WIFI_OTA    "Linh Anh"
#define DEFAULT_PASS_OTA    "08032025"
//
// DISPLAY 1 (10 digit)
//
#define DISP1_DATA 0
#define DISP1_CLK  4
#define DISP1_LAT  5


#define UPDATE_PIN 15
#define CAM_BIEN_S5 32
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


//====================================================
// Character -> 7 Segment
// bit:
// DP G F E D C B A
//====================================================
int trang_thai_cam_bien = KHONG_CO_HANG;
int total_target = 1000;
int now_target = 12;
int now_actual = 0;
int hieu_suat = 10;
int so_hang_trong_moi_hop = 4;
int gio_bat_dau = 8, phut_bat_dau = 0, gio_giai_lao_0 = 12, phut_giai_lao_0 = 0, thoi_gian_giai_lao_0 = 60, gio_giai_lao_1 = 16, phut_giai_lao_1 = 0, thoi_gian_giai_lao_1 = 60;
int gio_ket_thuc = 0, phut_ket_thuc = 0;
int so_phut_lam_viec_thuc_te = 0;
int so_phut_hien_tai = 0;
int so_phut_da_nghi_giai_lao_0 = 0, so_phut_da_nghi_giai_lao_1 = 0;
int gio_hien_tai = 0, phut_hien_tai = 0, giay_hien_tai = 0, ngay_hien_tai = 0, thang_hien_tai = 0, nam_hien_tai = 0;
String Model = "DCV584";
int trang_thai_display_1 = 0;

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
    Serial.println("Step 2");
    struct tm timeinfo;

    timeout = millis();

    while (!getLocalTime(&timeinfo))
    {
        Serial.println("Step 3");
        delay(500);
        Serial.print("*");

        if (millis() - timeout > 10000)
        {
            Serial.println("\nNTP Failed");
            return false;
        }
    }
    Serial.println("Step 4");
    DateTime ntpTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );
    Serial.println("Step 5");
    rtc.adjust(ntpTime);

    Serial.println("\nRTC Updated");

    getRTC();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    return true;
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


//====================================================
// Display 1
//====================================================

void display1(const String& text)
{
    displayString(
        text,
        DISPLAY1_DIGITS,
        DISP1_DATA,
        DISP1_CLK,
        DISP1_LAT);
}


//====================================================
// Display 2
//====================================================

void display2(const String& text)
{
    displayString(
        text,
        DISPLAY2_DIGITS,
        DISP2_DATA,
        DISP2_CLK,
        DISP2_LAT);
}
String to4DigitString(int value)
{
    value = constrain(value, 0, 9999);

    return String(value / 1000) +
           String((value / 100) % 10) +
           String((value / 10) % 10) +
           String(value % 10);
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
    pinMode(UPDATE_PIN, INPUT_PULLUP);
    pinMode(CAM_BIEN_S5, INPUT_PULLUP);
    if ((digitalRead(UPDATE_PIN) == LOW) )
    {
      WiFi.begin(DEFAULT_WIFI_OTA, DEFAULT_PASS_OTA);
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
        syncRTCfromInternet();
        display1("SUC-T     ");
    }
    
    delay(2000);
}

//====================================================
// Loop
//====================================================

void loop()
{
  getRTC();
  if ((digitalRead(UPDATE_PIN) == LOW) )
  {
      WiFi.begin(DEFAULT_WIFI_OTA, DEFAULT_PASS_OTA);
      while (WiFi.status() != WL_CONNECTED) 
      {
        Serial.print(".");
        delay(50);
        display1(FW_VERSION);
        display1("----------------");
      }
      display1(FW_VERSION);
      display1("ABCDEF123456789");
      checkAndUpdateFirmware();
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

  String msg = to4DigitString(total_target) + to4DigitString(now_target) + to4DigitString(now_actual) + to4DigitString(hieu_suat);

  display2(msg);
  
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
    now_actual = now_actual + so_hang_trong_moi_hop;
    delay(1000);
    trang_thai_cam_bien = KHONG_CO_HANG;
    trang_thai_display_1 += 1;
    if (trang_thai_display_1 > 2)
    {
        trang_thai_display_1 = 0;
    }
  }

  



}