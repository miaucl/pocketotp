#include "Adafruit_ThinkInk.h"
#include <FlashAsEEPROM.h>
#include "TOTP.h"
#include "sha1.h"
#include <Base32-Decode.h>
#include <TimeLib.h>
#include <Wire.h>
#include "ArduinoLowPower.h"
#include "RTClib.h"

#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_THINKINK // detects if compiling for
                                                // Feather RP2040 ThinkInk
#define EPD_DC PIN_EPD_DC       // ThinkInk 24-pin connector DC
#define EPD_CS PIN_EPD_CS       // ThinkInk 24-pin connector CS
#define EPD_BUSY PIN_EPD_BUSY   // ThinkInk 24-pin connector Busy
#define SRAM_CS -1              // use onboard RAM
#define EPD_RESET PIN_EPD_RESET // ThinkInk 24-pin connector Reset
#define EPD_SPI &SPI1           // secondary SPI for ThinkInk
#else
#define EPD_DC 10
#define EPD_CS 9
#define EPD_BUSY -1
#define SRAM_CS 6
#define EPD_RESET -1
#define EPD_SPI &SPI // primary SPI
#endif

#define COUNT_ENTRIES 24
#define MAX_LABEL 16
#define MAX_SECRET 24
#define EEPROM_BLOCK (MAX_LABEL + MAX_SECRET)
#define PERSISTANT true
#define MASKED false
#define DISPLAY_WIDTH 296
#define DISPLAY_HEIGHT 128
#define PAGE_PADDING_TOP 46
#define PAGE_PADDING_BOTTOM 24
#define ENTRY_DRAW_HEIGHT ((DISPLAY_WIDTH - PAGE_PADDING_TOP - PAGE_PADDING_BOTTOM) / (COUNT_ENTRIES / 3))
#define BUTTON_PAGE_1 11
#define BUTTON_PAGE_2 12
#define BUTTON_PAGE_3 13
#define VBATPIN A7
#define BAT_LEVEL_HIGH 4.0
#define BAT_LEVEL_MEDIUM 3.5
#define BAT_LEVEL_LOW 3.2
#define DEEP_SLEEP_TIME (3 * 60 * 1000)
#define RTC_GND 5

ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, EPD_SPI);

enum Page {PAGE_1=0, PAGE_2, PAGE_3};
Page selected_page = PAGE_1;

enum BatteryState {NO_BATTERY=0, LOW_BATTERY, MEDIUM_BATTERY, HIGH_BATTERY, USB_BATTERY};
BatteryState battery_state = USB_BATTERY;

char c;
int ci;
const byte char_length = 128;
char char_data[char_length + 1];

int eeprom_address = 0;

int last_refresh = 0;

uint8_t hmac_key[MAX_SECRET + 1];
TOTP totp(hmac_key, 0);
//DW7G2JB6EWMREYY4NA7HUV234SQUW5SS

RTC_DS3231 rtc;

void setup() 
{
  Serial.begin(115200);

  //while (!Serial) delay(10);

  pinMode(BUTTON_PAGE_1, INPUT_PULLUP);
  pinMode(BUTTON_PAGE_2, INPUT_PULLUP);
  pinMode(BUTTON_PAGE_3, INPUT_PULLUP);

  pinMode(RTC_GND, OUTPUT);
  digitalWrite(RTC_GND, LOW);

  delay(5000);

  if (!rtc.begin()) 
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) Serial.println("RTC lost power, provide time!");

  time_t t = rtc.now().unixtime();
  // Serial.print("Unix = ");
  // Serial.println(t);
  // DateTime n = rtc.now();
  // Serial.print(n.year(), DEC);
  // Serial.print('/');
  // Serial.print(n.month(), DEC);
  // Serial.print('/');
  // Serial.print(n.day(), DEC);
  // Serial.print(" ");
  // Serial.print(n.hour(), DEC);
  // Serial.print(':');
  // Serial.print(n.minute(), DEC);
  // Serial.print(':');
  // Serial.print(n.second(), DEC);
  // Serial.println();

  setTime(t);

  // uint8_t hmacKey[] = {0x1d, 0xbe, 0x6d, 0x24, 0x3e, 0x25, 0x99, 0x12, 0x63, 0x1c, 0x68, 0x3e, 0x7a, 0x57, 0x5b, 0xe4, 0xa1, 0x4b, 0x76, 0x52};     
  // TOTP totp = TOTP(hmacKey, 20);
  // Serial.println(totp.getCode(1705785694));

  // float measuredvbat = analogRead(VBATPIN) * 2 * 3.3 / 1024;
  // Serial.println(measuredvbat);
  // if (measuredvbat >= BAT_LEVEL_HIGH) battery_state = HIGH_BATTERY;
  // else if (measuredvbat >= BAT_LEVEL_MEDIUM) battery_state = MEDIUM_BATTERY;
  // else if (measuredvbat >= BAT_LEVEL_LOW) battery_state = LOW_BATTERY;
  // else battery_state = NO_BATTERY;

  display.begin(THINKINK_GRAYSCALE4);
  display.clearBuffer();
  display.setFont();

  drawRefreshHint();
  drawPageLabels(selected_page);
  drawTitle();
  drawTime();
  drawPageEntries(selected_page);
  drawBatteryState();
  display.display();
}

void loop() 
{
  if (Serial.available() > 0) 
  {
    // read first char
    c = Serial.read();
    switch (c)
    {
      case 't':
        readTimestamp();
        break;
      case 'n':
        readEntry();
        break;
      case 'c':
        clearAll();
        break;
      case 'p':
        printEntries();
        break;
    }
  }

  // Switch page
  if (digitalRead(BUTTON_PAGE_1) == LOW && selected_page != PAGE_1)
  {
    selected_page = PAGE_1;
    Serial.println("Select page 1");
    drawPageLabels(selected_page);
    refreshDraw();
  }
  else if (digitalRead(BUTTON_PAGE_2) == LOW && selected_page != PAGE_2)
  {
    selected_page = PAGE_2;
    Serial.println("Select page 2");
    drawPageLabels(selected_page);
    refreshDraw();
  }
  else if (digitalRead(BUTTON_PAGE_3) == LOW && selected_page != PAGE_3)
  {
    selected_page = PAGE_3;
    Serial.println("Select page 3");
    drawPageLabels(selected_page);
    refreshDraw();
  }

  // Refresh every 30s
  if (now() / 30 > last_refresh)
  {
    last_refresh = now() / 30;
    // Serial.print("Last refresh: ");
    // Serial.println(last_refresh);
    
    // Refresh
    refreshDraw();
  }

  // Stop after some time
  if (millis() > DEEP_SLEEP_TIME) LowPower.deepSleep();
}

/* Read */

int readSegment()
{
  ci = -1;
  c = Serial.read();
  while (c != ':' && c != '\n')
  {
    char_data[++ci] = c;

    if (ci == char_length)
    {
      Serial.println("entry data too long");
      return -1;
    }
    c = Serial.read();
  }
  char_data[++ci] = '\0';
  return ci;
}

void readEntry()
{
  if (Serial.read() != '=')
  {
    Serial.println("'=' missing after 'n'");
    return;
  }
  if (readSegment() == -1) return;
  eeprom_address = (atoi(char_data) - 1) * EEPROM_BLOCK;
  if (eeprom_address >= COUNT_ENTRIES * (MAX_LABEL + MAX_SECRET))
  {
    Serial.println("Only entries from 1 to 24 are allowed");
    return;
  }
  if (readSegment() == -1) return;
  writeLabelToEEPROM();
  if (readSegment() == -1) return;
  writeSecretToEEPROM();
  if (PERSISTANT) EEPROM.commit();
  Serial.println("Entry stored successfully! List entries with command 'p'.");
}

void clearAll()
{
  for (int i = 0; i<COUNT_ENTRIES; i++)
  {
    eeprom_address = i * EEPROM_BLOCK;
    ci = 1;
    char_data[0] = '\0';
    writeLabelToEEPROM();
  }
  if (PERSISTANT) EEPROM.commit();
  Serial.println("All entries cleared.");
}

void readTimestamp()
{
  if (Serial.read() != '=')
  {
    Serial.println("'=' missing after 't'");
    return;
  }
  if (readSegment() == -1) return;
  time_t t = (time_t)atol(char_data);
  setTime(t);
  rtc.adjust(DateTime(t));
  Serial.print("Unix timestamp set to: ");
  Serial.println(now());
}

/* Read from / write to EEPROM */

void writeLabelToEEPROM()
{
  if (ci > MAX_LABEL)
  {
    Serial.println("Label is too long!");
    return;
  }
  for (int i=0; i<MAX_LABEL; i++)
  {
    if (i<ci) EEPROM.write(eeprom_address + i, char_data[i]);
    else EEPROM.write(eeprom_address + i, '\0');
  }
}

void readLabelFromEEPROM()
{
  for (int i=0; i<char_length+1; i++) 
  {
    if (i<MAX_LABEL) char_data[i] = char(EEPROM.read(eeprom_address + i));
    else char_data[i] = '\0';
  }
}

void writeSecretToEEPROM()
{
  int maxout = base32decode(char_data, NULL, 0) + 1;

  if (maxout > MAX_SECRET)
  {
    Serial.println("Secret is too long!");
    return;
  }

  base32decode(char_data, hmac_key, maxout);

  for (int i=0; i<MAX_SECRET; i++)
  {
    if (i<ci) EEPROM.write(eeprom_address + MAX_LABEL + i, hmac_key[i]);
    else EEPROM.write(eeprom_address + MAX_LABEL + i, '\0');
  }
}

void readSecretFromEEPROM()
{
  for (int i=0; i<MAX_SECRET+1; i++) 
  {
    if (i<MAX_SECRET) hmac_key[i] = char(EEPROM.read(eeprom_address + MAX_LABEL + i));
    else hmac_key[i] = '\0';
  }
}

int readSecretLengthFromEEPROM()
{
  for (int i=0; i<MAX_SECRET; i++) 
  {
    if (char(EEPROM.read(eeprom_address + MAX_LABEL + i)) == '\0')  return i;
  }
  return MAX_SECRET;
}

/* Print */

void printStartupMessage()
{
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println("PockeTOTP started!");
  Serial.println("––––––––––––––––––");
  Serial.println();
  Serial.println("To configure the PockeTOTP, following options are available:");
  Serial.println();
  Serial.println("Set time:\t\tt=<unix timestamp>\\n");
  Serial.print("Write entry (n=1..24):\te=n:<label(");Serial.print(MAX_LABEL);Serial.print(")>:<secret(");Serial.print(MAX_SECRET);Serial.print(")>\\n");
  Serial.println("Clear entry (n=1..24):\te=n::\\n");
  Serial.println("Clear all:\t\tc\\n");
  Serial.println("Print all:\t\tp\\n");
  Serial.println();
  Serial.println("A helpful tool to decode QR codes to extract the secret is https://webqr.com");
  Serial.println("--> otpauth://totp/{label}?secret={secretKeyBase32Encoded}&issuer={issuer}");
  if (!EEPROM.isValid())
  {
    Serial.println();
    Serial.println("Start by setting some entries...");
  }
}

void printEntries()
{
  Serial.println("Entries");
  Serial.println("-------");
  for (int entry = 0; entry<COUNT_ENTRIES; entry++)
  {
    Serial.print(entry);
    Serial.print(": ");
    eeprom_address = entry * EEPROM_BLOCK;
    readLabelFromEEPROM();
    if (char_data[0] != '\0')
    {
      for (int i = 0; i<MAX_LABEL; i++) Serial.print(char_data[i]);
      Serial.print(" ");
      readSecretFromEEPROM();
      Serial.print(" ");
      totp = TOTP(hmac_key, readSecretLengthFromEEPROM());
      Serial.print(totp.getCode(now()));
    }
    Serial.println();
  }
}

/* Draw */

void drawTitle()
{
  display.fillRect(0, 0, 24, 128, EPD_BLACK);

  display.setRotation(3);
  display.setTextColor(EPD_LIGHT);
  display.setCursor(12, 2);
  display.setTextSize(2);
  display.print("PockeTOTP");
  display.setTextSize(1);
  display.setRotation(0);
}

void drawTime()
{
  char time_buffer[10];
  int16_t x = 0, y = 0;
  int16_t x1, y1;
  uint16_t w, h;

  sprintf(time_buffer, "UTC %02d:%02d", hour(), minute());
  display.getTextBounds(time_buffer, x, y, &x1, &y1, &w, &h); // calc width of string
  display.setRotation(3);
  display.setTextColor(EPD_BLACK);
  display.setCursor(64 - w/2, 26);
  display.fillRect(64 - w/2, 26, w, 8, EPD_WHITE);
  display.print(time_buffer);
  display.setRotation(0);
}

void drawRefreshHint()
{
  int16_t x = 0, y = 0;
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("Press to refresh", x, y, &x1, &y1, &w, &h); // calc width of string
  display.setRotation(3);
  display.setTextColor(EPD_DARK);
  display.setCursor(114 - w, 284);
  display.print("Press to refresh");
  display.drawLine(123, 296, 123, 288, EPD_DARK);
  display.drawLine(123, 288, 118, 288, EPD_DARK);
  display.setRotation(0);
}


void drawBatteryState()
{
  display.fillRect(2, DISPLAY_WIDTH - 10, 14, 8, EPD_WHITE);
  if (battery_state != USB_BATTERY)
  {
    display.setRotation(3);
    display.fillRoundRect(2, DISPLAY_WIDTH - 10, 14, 8, 2, EPD_BLACK);
    display.fillRect(15, DISPLAY_WIDTH - 8, 2, 4, EPD_BLACK);
    for (int i = 0; i<battery_state; i++) display.fillRect(4*(i + 1), DISPLAY_WIDTH - 8, 2, 4, EPD_WHITE);
    display.setRotation(0);
  }
  display.setTextColor(EPD_DARK);
  display.setCursor(18, DISPLAY_WIDTH - 4);
}


void drawPageLabels(Page selected)
{
  display.setRotation(0);
  display.fillRect(24, 0, DISPLAY_WIDTH - 42, 14, EPD_WHITE);

  display.setTextColor(EPD_DARK);

  display.setCursor(58, 2);
  display.print("Page 1");
  display.drawLine(47, 5, 52, 5, EPD_DARK);
  display.drawLine(47, 4, 47, 0, EPD_DARK);
  if (selected == PAGE_1) display.drawLine(58, 12, 92, 12, EPD_BLACK);

  display.setCursor(146, 2);
  display.print("Page 2");
  display.drawLine(135, 5, 140, 5, EPD_DARK);
  display.drawLine(135, 4, 135, 0, EPD_DARK);
  if (selected == PAGE_2) display.drawLine(146, 12, 180, 12, EPD_BLACK);

  display.setCursor(234, 2);
  display.print("Page 3");
  display.drawLine(223, 5, 228, 5, EPD_DARK);
  display.drawLine(223, 4, 223, 0, EPD_DARK);
  if (selected == PAGE_3) display.drawLine(234, 12, 268, 12, EPD_BLACK);
}

void drawPageEntries(int page)
{
  display.setRotation(3);
  display.fillRect(0, PAGE_PADDING_TOP, DISPLAY_HEIGHT - 14, DISPLAY_WIDTH - PAGE_PADDING_TOP - PAGE_PADDING_BOTTOM, EPD_WHITE);
  display.setRotation(1);

  for (int entry = 0; entry<COUNT_ENTRIES/3; entry++)
  {
    eeprom_address = (entry + page * COUNT_ENTRIES/3) * EEPROM_BLOCK;
    readLabelFromEEPROM();
    if (char_data[0] != '\0')
    {
      drawEntryLabel(entry);
      readSecretFromEEPROM();
      totp = TOTP(hmac_key, readSecretLengthFromEEPROM());
      drawEntryTOTP(entry);
    }
  }
}

void drawEntryLabel(int index)
{
  display.setRotation(3);
  display.setTextColor(EPD_BLACK);
  display.setCursor(0, PAGE_PADDING_TOP + ENTRY_DRAW_HEIGHT * index);
  display.print(char_data);
  display.setRotation(0);
}

void drawEntryTOTP(int index)
{
  display.setRotation(3);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(2);
  display.setCursor(DISPLAY_HEIGHT - 96, PAGE_PADDING_TOP + ENTRY_DRAW_HEIGHT * index + 10);
  display.print(totp.getCode(now()));
  display.setRotation(0);
  display.setTextSize(1);
}

void refreshDraw()
{
  drawTime();
  drawPageEntries(selected_page);
  drawBatteryState();
  display.display();
}
