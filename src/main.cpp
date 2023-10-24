// ESP32 HW VSPI
// #define V_SPI_D 23   // defined in ESP32 VSPI hardware
// #define V_SPI_CLK 18 // defined in ESP32 VSPI hardware
#define V_SPI_CS0 5  // must be 5 for ESP32 VSPI
#define SPI_DC 27    // any GPIO pin will work
#define SPI_RESET 26 // any GPIO pin will work

// #define DCSBIOS // turns dcs bios on, leave off for serial port monitoring

#ifdef DCSBIOS
#define DCSBIOS_DEFAULT_SERIAL // Use for STM32/ESP32
#include "DcsBios.h"
#endif
#include <Arduino.h>
#include <U8g2lib.h>
#include <U8glib.h>
#include <SPI.h>
#include <Wire.h>

// U8G2_R2 rotates 180deg - https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#rotation
// (rotation, cs, dc [, reset]) [full framebuffer, size = 512 bytes]
U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g2(U8G2_R2, V_SPI_CS0, SPI_DC, SPI_RESET);

// define the correct gemoetry for rows based on font
#define DED_FONT u8g2_font_9x15_m_symbols // w=9, h=15, A=10
const uint8_t textWidth = 9;              // 256px / 26 chars = 9.8px
const uint8_t textHeight = 15;            // 64px / 5 lines = 12.8px
const uint8_t rowHeight = 12;             // using 12 to cram letters closer vertically
const uint8_t row1 = rowHeight;
const uint8_t row2 = rowHeight * 2;
const uint8_t row3 = rowHeight * 3;
const uint8_t row4 = rowHeight * 4;
const uint8_t row5 = rowHeight * 5;

//  initialize empty ded lines
char *line1 = (char *)"";
char *line2 = (char *)"";
char *line3 = (char *)"";
char *line4 = (char *)"";
char *line5 = (char *)"";

/**
 * @brief replace a char on the screen with a new one
 *
 * @param row the bottom pixel of the row on which the char exists
 * @param index the number of the char in the row to replace (starting from 1)
 * @param newGlyph the new char that will be drawn in that location
 */
void replaceChar(uint8_t row, int index, uint16_t newGlyph)
{
  u8g2.setDrawColor(0);                                                  // color to blank
  u8g2.drawBox(textWidth * index, row - textHeight, textWidth, row + 1); // create a box around the old char
  u8g2.setDrawColor(1);                                                  // set the color back to on
#ifndef DCSBIOS
  Serial.println("REPLACED - row:" + String(row) + ", index:" + String(index) + " newGlyph:" + newGlyph);
#endif
  u8g2.drawGlyph(textWidth * index, row, newGlyph); // draw new utf-8 symbol
}

/**
 * @brief Draw a highlighted character on the screen
 *
 * @param row the bottom pixel of the row on which the char exists
 * @param index the number of the char in the row to replace (starting from 1)
 * @param hChar the char that will be drawn in the highlighted cell
 */
void highlightChar(uint8_t row, int index, char hChar)
{
  char hChars[2] = {hChar, '\0'};
#ifndef DCSBIOS
  Serial.println("HIGHLIGHT - row:" + String(row) + " index:" + String(index) + " char: " + hChars);
#endif
  u8g2.drawButtonUTF8(textWidth * index - textWidth, row, U8G2_BTN_INV, 0, 0, 0, hChars);
}

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
  // Process the dedLine and draw the first 25 characters on the screen in the correct row
  char firstTwentySix[26]; // char array that contains the string to be printed (before formatting)
  strncpy(firstTwentySix, dedLine, 26);
  firstTwentySix[25] = '\0'; // Null-terminate the string
#ifndef DCSBIOS
  Serial.println("DRAW - row:" + String(row) + " '" + firstTwentySix + "'");
#endif
  u8g2.drawStr(0, row, firstTwentySix);

  // Replace any matching characters with the corrected symbols
  int length = strlen(firstTwentySix);
  for (int i = 0; i < length; i++)
  {
    if (firstTwentySix[i] == 'o')
    {
      replaceChar(row, i, 0x00B0); // replace "o" with degree symbol
    }
    else if (firstTwentySix[i] == 'a')
    {
      replaceChar(row, i, 0x21D5); // replace "a" with up/down arrow symbol
    }
    else if (firstTwentySix[i] == '*')
    {
      highlightChar(row, i + 1, '*'); // highlight all "*"
    }
  }

  // Extract the last 3 characters from the dedLine, these contain the highlight maps
  char lastThree[3] = "";
  if (strlen(dedLine) > 26)
  {
    strncpy(lastThree, dedLine + 25, 3);
    lastThree[3] = '\0'; // Null-terminate the string
  }
  int lastThreeLen = strlen(lastThree);
#ifndef DCSBIOS
  Serial.println("LAST3 - row:" + String(row) + " len:" + lastThreeLen + " '" + String(lastThree) + "'");
#endif

  // Highlight characters in firstTwentySix based on lastThree of the dedLine
  int index = 1;                         // index is the position in the first 26 chars starting from 1
  for (int i = 0; i < lastThreeLen; i++) // iterate through each of the control chars
  {
    for (int b = 0; b <= 7; b++) // read each bit in the control char
    {
      if (bitRead(lastThree[i], b) == 1)
      {
        // the bit is 1, highlight that char
        highlightChar(row, index, firstTwentySix[index - 1]);
      }
      index++;
    }
  }
}

/**
 * @brief prints test CNI Page of the DED
 */
void cniPage()
{
  u8g2.firstPage();
  do
  {
    drawDedLine(row1, " UHF  305.00  STPT a  1  "); // a is up/dwn arrow
    drawDedLine(row2, "                         ");
    drawDedLine(row3, " VHF  127.00   11:53:36  ");
    drawDedLine(row4, "                         ");
    drawDedLine(row5, " M   4   1337      T  1X ");
  } while (u8g2.nextPage());
}
/**
 * @brief prints test Steerpoint Page of the DED
 */
void stptPage()
{
  u8g2.firstPage();
  do
  {
    drawDedLine(row1, "      STPT *  1*a MAN    "); // * == HL, a == up/dwn
    drawDedLine(row2, "   LAT  N 43o03.400'     "); // o == degrees
    drawDedLine(row3, "   LNG  E040o59.713'     "); // o == degrees
    drawDedLine(row4, "  ELEV    5000FT         ");
    drawDedLine(row5, "   TOS  11:47:39         ");
  } while (u8g2.nextPage());
}
/**
 * @brief prints test List Page of the DED
 */
void listPage()
{
  u8g2.firstPage();
  do
  {
    // pulled from dcs-bios lua env. ex: `return hub.getSimString("F-16C_50/DED_LINE_3")`
    drawDedLine(row1, "          LIST           ");
    drawDedLine(row2, "1DEST 2BNGO 3VIP  RINTG  A\x10\x04"); // 1,2,3,R should be HL
    drawDedLine(row3, "4NAV  5MAN  6INS  EDLNK  A\x10\x04");
    drawDedLine(row4, "7CMDS 8MODE 9VRP  0MISC  A\x10\x04");
    drawDedLine(row5, "                         ");
  } while (u8g2.nextPage());
}
/**
 * @brief prints test com1 Page of the DED
 */
void com1Page()
{
  u8g2.firstPage();
  do
  {
    drawDedLine(row1, "     UHF     MAIN        ");
    drawDedLine(row2, "  305.00                 ");
    drawDedLine(row3, "             *305.00*    "); // asterics should be HL
    drawDedLine(row4, "  PRE   1 a      TOD     ");
    drawDedLine(row5, "     305.00       NB     ");
  } while (u8g2.nextPage());
}

#ifdef DCSBIOS
/**
 * @brief Callback for when new DCS BIOS line1 arrives
 */
void onDedLine1Change(char *newValue)
{
  line1 = newValue;
}
// https://dcs-bios.readthedocs.io/en/latest/code-snippets.html#stringbuffer-and-integerbuffer
DcsBios::StringBuffer<29> dedLine1Buffer(0x4500, onDedLine1Change);

/**
 * @brief Callback for when new DCS BIOS line2 arrives
 */
void onDedLine2Change(char *newValue)
{
  line2 = newValue;
}
DcsBios::StringBuffer<29> dedLine2Buffer(0x451e, onDedLine2Change);

/**
 * @brief Callback for when new DCS BIOS line3 arrives
 */
void onDedLine3Change(char *newValue)
{
  line3 = newValue;
}
DcsBios::StringBuffer<29> dedLine3Buffer(0x453c, onDedLine3Change);

/**
 * @brief Callback for when new DCS BIOS line4 arrives
 */
void onDedLine4Change(char *newValue)
{
  line4 = newValue;
}
DcsBios::StringBuffer<29> dedLine4Buffer(0x455a, onDedLine4Change);

/**
 * @brief Callback for when new DCS BIOS line5 arrives
 */
void onDedLine5Change(char *newValue)
{
  line5 = newValue;
}
DcsBios::StringBuffer<29> dedLine5Buffer(0x4578, onDedLine5Change);
#endif

/**
 * @brief Prints a Splash Screen, used during boot up
 */
void splashScreen()
{
#ifndef DCSBIOS
  Serial.println("SPLASH SCREEN");
#endif
  u8g2.firstPage();
  do
  {
    u8g2.drawStr(0, row1, " _______________________ ");
    u8g2.drawStr(0, row2, "|                       |");
    u8g2.drawStr(0, row3, "|     DED BY SOCKEYE    |");
    u8g2.drawStr(0, row4, "|     V303FG / 93FS     |");
    u8g2.drawStr(0, row5, "|_______________________|");
  } while (u8g2.nextPage());
}

/**
 * @brief Setup loop, run once on boot
 */
void setup()
{
#ifdef DCSBIOS
  DcsBios::setup();
#else
  Serial.begin(9600);
  Serial.println("Setup start");
#endif
  u8g2.begin();
  u8g2.setFont(DED_FONT); // https://github.com/olikraus/u8g2/wiki/fntgrpx11#8x13
  u8g2.setFontMode(1);    // https://github.com/olikraus/u8g2/wiki/u8g2reference#setfontmode
  u8g2.setDrawColor(1);   // https://github.com/olikraus/u8g2/wiki/u8g2reference#setdrawcolor
  splashScreen();
  delay(10 * 1000); // pause X secs for our sponsor. :-)
#ifndef DCSBIOS
  Serial.println("Setup done");
#endif
}

/**
 * @brief main loop
 */
void loop()
{
#ifdef DCSBIOS
  u8g2.firstPage();
  do
  {
    drawDedLine(row1, line1);
    drawDedLine(row2, line2);
    drawDedLine(row3, line3);
    drawDedLine(row4, line4);
    drawDedLine(row5, line5);
  } while (u8g2.nextPage());
  DcsBios::loop();
#else
  cniPage();
  delay(3 * 1000);
  com1Page();
  delay(3 * 1000);
  stptPage();
  delay(3 * 1000);
  listPage();
  delay(3 * 1000);
#endif
}