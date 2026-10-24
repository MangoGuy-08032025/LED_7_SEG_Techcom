#include <Arduino.h>
#include "ota_update.h"
#define DEFAULT_WIFI_OTA    "Linh Anh"
#define DEFAULT_PASS_OTA    "08032025"
//
// DISPLAY 1 (10 digit)
//
#define DISP1_DATA 0
#define DISP1_CLK  4
#define DISP1_LAT  5
#define UPDATE_PIN 15
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
int total_target = 1000;
int now_target = 500;
int now_actual = 100;
int hieu_suat = 0;
String Model = "DCV584";

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
        case 'V': seg = 0xC1; break;
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
    if ((digitalRead(UPDATE_PIN) == LOW) )
    {
      WiFi.begin(DEFAULT_WIFI_OTA, DEFAULT_PASS_OTA);
      while (WiFi.status() != WL_CONNECTED) 
      {
        Serial.print(".");
        delay(50);
        display1(FW_VERSION);
        display2("----------------");
      }
      display1(FW_VERSION);
      display2("ABCDEF123456789");
      checkAndUpdateFirmware();
    }
}

//====================================================
// Loop
//====================================================

void loop()
{
  if ((digitalRead(UPDATE_PIN) == LOW) )
  {
      WiFi.begin(DEFAULT_WIFI_OTA, DEFAULT_PASS_OTA);
      while (WiFi.status() != WL_CONNECTED) 
      {
        Serial.print(".");
        delay(50);
        display1("FW_VERSION");
        display1("----------------");
      }
      display1("FW_VERSION");
      display1("ABCDEF123456789");
      checkAndUpdateFirmware();
  }
  display1(Model);
  String hs = String(hieu_suat);
  while(hs.length() < 4)
  {
      hs = "0" + hs;
  }
  String display2_data = String(total_target) + String(now_target) + String(now_actual) + hs;
  display2(display2_data);
    // static uint32_t last = 0;
    // static int counter = 400;

    // if (millis() - last > 1000)
    // {
    //     last = millis();

    //     display1(String(counter));
    //     display2("TIME=" + String(counter));

    //     if (counter > 0)
    //         counter--;
    // }
}