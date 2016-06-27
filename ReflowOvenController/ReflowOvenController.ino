/*******************************************************************************
* Title: Reflow Oven Controller
* Version: 0.01
* Date: 2012/06/27
* Author: Yong GE 
* 
* Brief
* =====
* This is an example firmware for our Arduino compatible reflow oven controller. 
* The reflow curve used in this firmware is meant for lead-free profile 
* (it's even easier for leaded process!). You'll need to use the MAX6675
* library for Arduino if you are having a shield of v1.60 & above which can be 
* downloaded from our GitHub repository. Please check our wiki 
* (www.rocketscream.com/wiki) for more information on using this piece of code 
* together with the reflow oven controller shield. 
*
* Temperature (Degree Celcius)                 Magic Happens Here!
* 245-|                                               x  x  
*     |                                            x        x
*     |                                         x              x
*     |                                      x                    x
* 200-|                                   x                          x
*     |                              x    |                          |   x   
*     |                         x         |                          |       x
*     |                    x              |                          |
* 150-|               x                   |                          |
*     |             x |                   |                          |
*     |           x   |                   |                          | 
*     |         x     |                   |                          | 
*     |       x       |                   |                          | 
*     |     x         |                   |                          |
*     |   x           |                   |                          |
* 30 -| x             |                   |                          |
*     |<  60 - 90 s  >|<    90 - 120 s   >|<       90 - 120 s       >|
*     | Preheat Stage |   Soaking Stage   |       Reflow Stage       | Cool
*  0  |_ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ 
*                                                                Time (Seconds)
*
* This firmware owed very much on the works of other talented individuals as
* follows:
* ==========================================
* Brett Beauregard (www.brettbeauregard.com)
* ==========================================
* Author of Arduino PID library. On top of providing industry standard PID 
* implementation, he gave a lot of help in making this reflow oven controller 
* possible using his awesome library.
*
* ==========================================
* Limor Fried of Adafruit (www.adafruit.com)
* ==========================================
* Author of Arduino MAX6675 library. Adafruit has been the source of tonnes of
* tutorials, examples, and libraries for everyone to learn.
*
* Disclaimer
* ==========
* Dealing with high voltage is a very dangerous act! Please make sure you know
* what you are dealing with and have proper knowledge before hand. Your use of 
* any information or materials on this reflow oven controller is entirely at 
* your own risk, for which we shall not be liable. 
*
* Licences
* ========
* This reflow oven controller hardware and firmware are released under the 
* Creative Commons Share Alike v3.0 license
* http://creativecommons.org/licenses/by-sa/3.0/ 
* You are free to take this piece of code, use it and modify it. 
* All we ask is attribution including the supporting libraries used in this 
* firmware. 
*
* Required Libraries
* ==================
* - Arduino PID Library: 
*   >> https://github.com/br3ttb/Arduino-PID-Library
* - MAX31855 Library (for board v1.60 & above): 
*   >> https://github.com/rocketscream/MAX31855
* - MAX6675 Library (for board v1.50 & below):
*   >> https://github.com/adafruit/MAX6675-library
*
* Revision  Description
* ========  ===========
* 1.10      Arduino IDE 1.0 compatible.
* 1.00      Initial public release.
******************************************************************************/


/*
 # This sample codes is for testing the LCD12864 shield.
 # Editor : Mickey
 # Date   : 2013.11.27
 # Ver    : 0.1
 # Product: LCD12864 shield
 # SKU    : DFR0287
 # http://wiki.dfrobot.com.cn/index.php/(SKU:DFR0287)LCD12864_shield_%E5%85%BC%E5%AE%B9Arduino
*/


// Universal graphic lib 
#include "U8glib.h"
// MAX6675 lib 
#include "max6675.h"

// setup u8g object, please remove comment from one of the following constructor calls
// IMPORTANT NOTE: The complete list of supported devices is here: http://code.google.com/p/u8glib/wiki/device
// C12864 SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9, RST = 8
const int C12864SCK  = 13;
const int C12864MOSI = 11;
const int C12864CS   = 10;
const int C12864A0   =  9;
const int C12864RST  =  8;

U8GLIB_NHD_C12864 u8g(C12864SCK, C12864MOSI, C12864CS, C12864A0, C12864RST);    

// Max6675 module: DO on pin #4, CS on pin #5, CLK on pin #6 of Arduino UNO
// Other pins are capable to run this library, as long as digitalRead works on DO,
// and digitalWrite works on CS and CLK

const int thermoDO  = 4;
const int thermoCS  = 5;
const int thermoCLK = 6;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

#define KEY_NONE   0
#define KEY_PREV   1
#define KEY_NEXT   2
#define KEY_SELECT 3
#define KEY_BACK   4

uint8_t uiKeyCodeFirst  = KEY_NONE;
uint8_t uiKeyCodeSecond = KEY_NONE;
uint8_t uiKeyCode       = KEY_NONE;

int adc_key_in =  0;
int key        = -1;
int oldkey     = -1;


// Convert ADC value to key number
//         4
//         |
//   0 --  1 -- 3
//         |
//         2

int get_key(unsigned int input)
{   
    if (input < 100) return 0;
    else if (input < 300) return 1;
    else if (input < 500) return 2;
    else if (input < 700) return 3;
    else if (input < 900) return 4;    
    else  return -1;
}

void uiKeyStep(void) {
  
  adc_key_in = analogRead(0);    // read the value from the sensor  
  key = get_key(adc_key_in);     // convert into key press    
  if (key != oldkey)             // if keypress is detected
  {
    delay(50);                   // wait for debounce time
    adc_key_in = analogRead(0);  // read the value from the sensor  
    key = get_key(adc_key_in);   // convert into key press
    if (key != oldkey)                
    {            
      oldkey = key;
      if (key >=0)
      {
          // Serial.println(key);
          if ( key == 0 )
            uiKeyCodeFirst = KEY_BACK;
          else if ( key == 1 )
            uiKeyCodeFirst = KEY_SELECT;
          else if ( key == 2 )
            uiKeyCodeFirst = KEY_NEXT;
          else if ( key == 4 )
            uiKeyCodeFirst = KEY_PREV;
          else 
            uiKeyCodeFirst = KEY_NONE;
  
           uiKeyCode = uiKeyCodeFirst;           
      }
    }
  }
  delay(100);
}


/* some utility functions here 
 *  
 */

char* dtostr(char *str, double d) {
  sprintf(str, "%f", d);
  return str;
}

uint8_t    fontHeight  = 0;
u8g_uint_t screenWidth = 0;

void setup() {
  
  // rotate screen, if required
  u8g.setRot180();
  // 4x6 5x7 5x8 6x10 6x12 6x13
  u8g.setFont(u8g_font_6x12);
  // set upper left position for the string draw procedure
  u8g.setFontPosTop();
  
  fontHeight  = u8g.getFontAscent() - u8g.getFontDescent();
  screenWidth = u8g.getWidth();

  // init the serial
  Serial.begin(9600);
  Serial.println("MAX6675 test");
  
  // wait for MAX chip to stabilize
  delay(500);

}

char strUpThermoCelsius[8];
char strDnThermoCelsius[8];
const char *strUpThermoText = "Up: "; 
void loop() {  

  
  u8g.firstPage();  

  dtostrf(thermocouple.readCelsius(), 4, 2, strUpThermoCelsius); 

  do {
    // graphic commands to redraw the complete screen should be placed here  
    u8g.drawStr(0, 1, strUpThermoText);
    u8g.drawStr(u8g.getStrWidth(strUpThermoText), 1, strUpThermoCelsius);
  } while( u8g.nextPage() );
 
  delay(1000);
    
}
