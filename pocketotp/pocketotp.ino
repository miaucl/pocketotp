#include "Adafruit_ThinkInk.h"

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

ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, EPD_SPI);

#define COLOR1 EPD_BLACK
#define COLOR2 EPD_LIGHT
#define COLOR3 EPD_DARK

enum Page {PAGE_1=0, PAGE_2, PAGE_3};
Page selected_page = PAGE_1;


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  display.begin(THINKINK_GRAYSCALE4);
  display.clearBuffer();
  display.setFont();

  displayStartupSplash();

  Serial.println("PockeTOTP started!");
  delay(1000);

  display.clearBuffer();
  drawRefreshHint();
  drawTitle();
  drawPageLabels(selected_page);
  display.display();
}

void loop() {


}

void displayStartupSplash()
{
  int16_t x = 296/2, y = 128/2;
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("PockeTOTP", x, y, &x1, &y1, &w, &h); // calc width of new string
  display.setTextColor(EPD_BLACK);
  display.setTextSize(2);
  display.setCursor(x - w, y - h);
  display.print("PockeTOTP");
  display.setTextSize(1);
  display.display();
}

void drawRefreshHint()
{
  display.setRotation(1);
  display.setTextColor(EPD_DARK);
  display.setCursor(14, 2);
  display.print("Press to refresh");
  display.drawLine(3, 5, 8, 5, EPD_DARK);
  display.drawLine(3, 4, 3, 0, EPD_DARK);
  display.setRotation(0);
}

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

void drawPageLabels(Page selected)
{
  display.setTextColor(EPD_DARK);

  display.setCursor(58, 2);
  display.print("Page 1");
  display.drawLine(47, 5, 52, 5, EPD_DARK);
  display.drawLine(47, 4, 47, 0, EPD_DARK);
  if (selected == PAGE_1) display.drawLine(58, 12, 92, 12, EPD_DARK);

  display.setCursor(146, 2);
  display.print("Page 2");
  display.drawLine(135, 5, 140, 5, EPD_DARK);
  display.drawLine(135, 4, 135, 0, EPD_DARK);
  if (selected == PAGE_2) display.drawLine(146, 12, 180, 12, EPD_DARK);

  display.setCursor(234, 2);
  display.print("Page 3");
  display.drawLine(223, 5, 228, 5, EPD_DARK);
  display.drawLine(223, 4, 223, 0, EPD_DARK);
  if (selected == PAGE_3) display.drawLine(234, 12, 268, 12, EPD_DARK);
}
