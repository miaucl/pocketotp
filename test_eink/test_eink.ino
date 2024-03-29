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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Adafruit EPD full update test in mono & grayscale");
}

bool gray = false;
void loop() {

  // alternate modes!
  if (gray) {
    display.begin(THINKINK_GRAYSCALE4);
    Serial.println("Grayscale!");
  } else {
    display.begin(THINKINK_MONO);
    Serial.println("Monochrome!");
  }

  display.clearBuffer();
  display.setTextSize(3);
  display.setTextColor(EPD_BLACK);
  display.setCursor(20, 40);
  if (gray) {
    display.print("Grayscale");
  } else {
    display.print("Monochrome");
  }

  gray = !gray;

  display.display();
  delay(1000);

  display.clearBuffer();
  display.fillRect(display.width() / 4, 0, display.width() / 4,
                   display.height(), EPD_LIGHT);
  display.fillRect((display.width() * 2) / 4, 0, display.width() / 4,
                   display.height(), EPD_DARK);
  display.fillRect((display.width() * 3) / 4, 0, display.width() / 4,
                   display.height(), EPD_BLACK);
  display.display();
  delay(2000);

  Serial.println("Text demo");
  // large block of text
  display.clearBuffer();
  display.setTextSize(1);
  testdrawtext(
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur "
      "adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, "
      "fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor "
      "neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet "
      "ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a "
      "tortor imperdiet posuere. ",
      COLOR1);
  display.display();
  delay(2000);

  display.clearBuffer();
  for (int16_t i = 0; i < display.width(); i += 4) {
    display.drawLine(0, 0, i, display.height() - 1, COLOR1);
  }

  for (int16_t i = 0; i < display.height(); i += 4) {
    display.drawLine(display.width() - 1, 0, 0, i,
                     COLOR2); // on grayscale this will be mid-gray
  }
  display.display();
  delay(2000);
}

void testdrawtext(const char *text, uint16_t color) {
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextWrap(true);
  display.print(text);
}
