// ESP32 HW VSPI
// #define V_SPI_D 23   // defined in ESP32 VSPI hardware
// #define V_SPI_CLK 18 // defined in ESP32 VSPI hardware
#define V_SPI_CS0 5  // must be 5 for ESP32 VSPI
#define SPI_DC 27    // any GPIO pin will work
#define SPI_RESET 26 // any GPIO pin will work

#ifdef WIFI
#include "secrets.h"
#include <WiFi.h>
#endif

#ifdef DCSBIOS
#include "DcsBios.h"
#endif

#include <Arduino.h>
#include <U8g2lib.h>
#include <U8glib.h>
#include <SPI.h>
#include <Wire.h>

// https://github.com/DCS-Skunkworks/dcs-bios/blob/master/Scripts/DCS-BIOS/doc/control-reference.html
#define F_16C_50_DED_LINE_1_A 0x450a
#define F_16C_50_DED_LINE_2_A 0x4528
#define F_16C_50_DED_LINE_3_A 0x4546
#define F_16C_50_DED_LINE_4_A 0x4564
#define F_16C_50_DED_LINE_5_A 0x4582

// U8G2_R2 rotates 180deg - https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#rotation
// (rotation, cs, dc [, reset]) [full framebuffer, size = 512 bytes]
U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g2(U8G2_R2, V_SPI_CS0, SPI_DC, SPI_RESET);

// define the correct gemoetry for rows based on font
const int DED_LINE_LEN = 25;              // DED string is 25 chars wide
#define DED_FONT u8g2_font_9x15_m_symbols // w=9, h=15, A=10
const uint8_t textWidth = 9;              // 256px / 25 chars = 10.24px
const uint8_t textHeight = 15;            // 64px / 5 lines = 12.8px
const uint8_t rowHeight = 12;             // using 12 to cram letters closer vertically
const uint8_t row1 = rowHeight;
const uint8_t row2 = rowHeight * 2;
const uint8_t row3 = rowHeight * 3;
const uint8_t row4 = rowHeight * 4;
const uint8_t row5 = rowHeight * 5;

// initialize empty ded lines
char *line1 = (char *)"";
char *line2 = (char *)"";
char *line3 = (char *)"";
char *line4 = (char *)"";
char *line5 = (char *)"";

// timers
#ifdef CYCLES_PER_LOOP
long lastMillis = 0;
long loops = 0;
#endif

/**
 * @brief: Function to draw a line with characters replaced by UTF-8 symbols
 *
 *   DED has 26 characters, DCS-BIOS sends 29 for the each DED Line.
 *   Each line is segmented in blocks of 8, each of the 3 addresses: 26, 27 and 28 is a hex that, when converted to binary
 *   (and reversed) expresses the highlight on/off characters for the first 26.
 *
 * @param: row the pixel for the bottom of the row on which we will draw the line
 * @param: dedLine the char array with the line that we will print
 *
 **/
void drawDedLine(uint8_t row, const char dedLine[])
{
  // Extract the first 25 chars from the supplied dedLine char array
  char dedChars[DED_LINE_LEN];              // new char array that will contain only the string to be printed
  strncpy(dedChars, dedLine, DED_LINE_LEN); // pull the first 25 chars from dedLine into the dedChars var
  dedChars[DED_LINE_LEN - 1] = '\0';        // Null-terminate the string

  // Replace any matching characters with the corrected symbols
  for (int i = 0; i < DED_LINE_LEN; i++)
  {
    if (dedChars[i] == 'o')
    {
      dedChars[i] = 0x00B0; // replace with ascii degree symbol
    }
    else if (dedChars[i] == 'a')
    {
      dedChars[i] = 0x0020;                       // replace with ascii space, we'll fill in utf char below
      u8g2.drawGlyph(textWidth * i, row, 0x21D5); // draw new utf-8 up/down arrow glyph in that spot
    }
  }
  dedChars[DED_LINE_LEN - 1] = '\0'; // null terminate the dedChars array
  u8g2.drawStr(0, row, dedChars);

#ifdef DEBUG
  Serial.println("DRAW - row:" + String(row / rowHeight) + " '" + dedChars + "'");
#endif

  // Highlight dedChars (first 25) based on ctrlChars (last 4)
  unsigned long bitMap;
  memcpy(&bitMap, &dedLine[DED_LINE_LEN], 4);
  for (int i = 0; i < DED_LINE_LEN; i++)
  {
    if (bitRead(bitMap, i))
    {
      char hChar[2] = {dedChars[i], '\0'};
      // drawButtonUTF8() creates a solid box around a transparent char
      u8g2.drawButtonUTF8(textWidth * (i + 1) - textWidth, row, U8G2_BTN_INV, 0, 0, 0, hChar);
    }
  }
}

#ifdef DCSBIOS
/**
 * @brief Callback for when new DCS BIOS line1 arrives
 */
void onDedLine1Change(char *newValue)
{
  line1 = newValue;
}
DcsBios::StringBuffer<29> dedLine1Buffer(F_16C_50_DED_LINE_1_A, onDedLine1Change);

/**
 * @brief Callback for when new DCS BIOS line2 arrives
 */
void onDedLine2Change(char *newValue)
{
  line2 = newValue;
}
DcsBios::StringBuffer<29> dedLine2Buffer(F_16C_50_DED_LINE_2_A, onDedLine2Change);

/**
 * @brief Callback for when new DCS BIOS line3 arrives
 */
void onDedLine3Change(char *newValue)
{
  line3 = newValue;
}
DcsBios::StringBuffer<29> dedLine3Buffer(F_16C_50_DED_LINE_3_A, onDedLine3Change);

/**
 * @brief Callback for when new DCS BIOS line4 arrives
 */
void onDedLine4Change(char *newValue)
{
  line4 = newValue;
}
DcsBios::StringBuffer<29> dedLine4Buffer(F_16C_50_DED_LINE_4_A, onDedLine4Change);

/**
 * @brief Callback for when new DCS BIOS line5 arrives
 */
void onDedLine5Change(char *newValue)
{
  line5 = newValue;
}
DcsBios::StringBuffer<29> dedLine5Buffer(F_16C_50_DED_LINE_5_A, onDedLine5Change);
#endif

/**
 * @brief Prints a Splash Screen, used during boot up
 */
void splashScreen()
{
  Serial.println("SPLASH SCREEN");
  u8g2.clearBuffer();
  u8g2.drawStr(0, row1, " _______________________ ");
  u8g2.drawStr(0, row2, "|                       |");
  u8g2.drawStr(0, row3, "| WiFi DED BY SOCKEYE   |");
  u8g2.drawStr(0, row4, "|     V303FG / 93FS     |");
  u8g2.drawStr(0, row5, "|_______________________|");
  u8g2.sendBuffer();
}

void setup()
{
#ifdef DCSBIOS
  DcsBios::setup();
#endif
  Serial.begin(115200);
  u8g2.begin();
  u8g2.setFont(DED_FONT);
  u8g2.setFontMode(1);
  u8g2.setDrawColor(1);
  splashScreen();
  delay(3 * 1000);
}

void loop()
{
#ifdef DCSBIOS
  DcsBios::loop();
#endif

  u8g2.clearBuffer();
  drawDedLine(row1, line1);
  drawDedLine(row2, line2);
  drawDedLine(row3, line3);
  drawDedLine(row4, line4);
  drawDedLine(row5, line5);
  u8g2.sendBuffer();

#ifdef CYCLES_PER_LOOP
  long currentMillis = millis();
  loops++;

  if (currentMillis - lastMillis > 1000)
  {
    Serial.print(loops);
    Serial.println("Hz");

    lastMillis = currentMillis;
    loops = 0;
  }
#endif
}
