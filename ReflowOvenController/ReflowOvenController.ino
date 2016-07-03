#include <U8glib.h>

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
// Brett Beauregard's PID lib
#include "PID_v1.h"

// IMPORTANT NOTE: The complete list of supported devices is here: http://code.google.com/p/u8glib/wiki/device
// C12864 SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9, RST = 8
#define C12864SCK   13
#define C12864MOSI  11
#define C12864CS    10
#define C12864A0     9
#define C12864RST    8

// setup u8g object, please remove comment from one of the following constructor calls
U8GLIB_NHD_C12864 u8g(C12864SCK, C12864MOSI, C12864CS, C12864A0, C12864RST);    

// Max6675 module: DO = 4, CS = 5, CLK = 6 of Arduino UNO
// Other pins are capable to run this library, as long as digitalRead works on DO,
// and digitalWrite works on CS and CLK

// First thermo
#define thermo1DO    2
#define thermo1CS    3
#define thermo1CLK   4
// Second thermo
#define thermo2DO    5
#define thermo2CS    6
#define thermo2CLK   12

// ***** TYPE DEFINITIONS *****
typedef enum REFLOW_STATE
{
  REFLOW_STATE_IDLE,
  REFLOW_STATE_PREHEAT,
  REFLOW_STATE_SOAK,
  REFLOW_STATE_REFLOW,
  REFLOW_STATE_COOL,
  REFLOW_STATE_COMPLETE,
  REFLOW_STATE_TOO_HOT,
  REFLOW_STATE_ERROR
} reflowState_t;

// ***** CONSTANTS *****
#define TEMPERATURE_ROOM        50
#define TEMPERATURE_SOAK_MIN   150
#define TEMPERATURE_SOAK_MAX   200
#define TEMPERATURE_REFLOW_MAX 250
#define TEMPERATURE_COOL_MIN   100
#define SENSOR_SAMPLING_TIME  1000
#define SOAK_TEMPERATURE_STEP    5
#define SOAK_MICRO_PERIOD     9000

// ***** PRE-HEAT STAGE *****
#define PID_KP_PREHEAT         100
#define PID_KI_PREHEAT       0.025
#define PID_KD_PREHEAT          20

// ***** SOAKING STAGE *****
#define PID_KP_SOAK            300
#define PID_KI_SOAK           0.05
#define PID_KD_SOAK            250

// ***** REFLOW STAGE *****
#define PID_KP_REFLOW          300
#define PID_KI_REFLOW         0.05
#define PID_KD_REFLOW          350
#define PID_SAMPLE_TIME       1000

// ***** PID window *****
#define PID_WINDOW_SIZE       2000

MAX6675 mainThermoCouple(thermo1CLK, thermo1CS, thermo1DO);
MAX6675 auxiThermoCouple(thermo2CLK, thermo2CS, thermo2DO);

uint8_t    fontHeight  = 0;
u8g_uint_t screenWidth = 0;

// ***** PID CONTROL VARIABLES *****
double mainSetpoint, mainInput, mainOutput;
double auxiSetpoint, auxiInput, auxiOutput;

double kp = PID_KP_PREHEAT;
double ki = PID_KI_PREHEAT;
double kd = PID_KD_PREHEAT;

unsigned long pidNextCheck;
unsigned long pidNextRead;
unsigned long windowStartTime;
unsigned long timerSoak;

// Reflow oven controller state machine state variable
reflowState_t reflowState;

// Specify PID control interface
PID mainOvenPID(&mainInput, &mainOutput, &mainSetpoint, kp, ki, kd, DIRECT);
PID auxiOvenPID(&auxiInput, &auxiOutput, &auxiSetpoint, kp, ki, kd, DIRECT);

char strThermoCelsius[8];
char strPidOutputBuf[16];
const char *strThermo1Text = "main "; 
const char *strThermo2Text = "auxi "; 

void setup() {

  // rotate screen, if required
  u8g.setRot180();
  // 4x6 5x7 5x8 6x10 6x12 6x13
  u8g.setFont(u8g_font_9x15);
  // set upper left position for the string draw procedure
  u8g.setFontPosTop();
  
  fontHeight  = u8g.getFontAscent() - u8g.getFontDescent();
  screenWidth = u8g.getWidth();

  // Initialize time keeping variable
  pidNextCheck = millis();
  // Initialize thermocouple reading variable
  pidNextRead  = millis();

  reflowState == REFLOW_STATE_IDLE;
  
  // wait for MAX chip to stabilize
  delay(500);

}


void loop() {  

  // Current time
  unsigned long timeNow;

  // Time to read thermocouple?
  if (millis() > pidNextRead)
  {
    // Read thermocouple next sampling period
    pidNextRead += SENSOR_SAMPLING_TIME;
    // Read current temperature
    mainInput = mainThermoCouple.readCelsius();
    auxiInput = auxiThermoCouple.readCelsius();
  }

  if (millis() > pidNextCheck)
  {
    // Check input in the next seconds
    pidNextCheck += PID_SAMPLE_TIME;
//    if ( isnan(mainInput) || isnan(auxiInput) )
    if ( isnan(mainInput)  )
    {
      reflowState = REFLOW_STATE_ERROR;
      goto show_message;
    }
  }

  // Reflow oven controller state machine
  switch (reflowState)
  {
  case REFLOW_STATE_IDLE:
    // If oven temperature is still above room temperature
    if (mainInput >= TEMPERATURE_ROOM)
    {
      reflowState = REFLOW_STATE_TOO_HOT;
    }
    else
    {
      // Initialize PID control window starting time
      windowStartTime = millis();
      // Ramp up to minimum soaking temperature
      mainSetpoint = TEMPERATURE_SOAK_MIN;
      // Tell the PID to range between 0 and the full window size
      mainOvenPID.SetOutputLimits(0, PID_WINDOW_SIZE);
      mainOvenPID.SetSampleTime(PID_SAMPLE_TIME);
      // Turn the PID on
      mainOvenPID.SetMode(AUTOMATIC);
      // Proceed to preheat stage
      reflowState = REFLOW_STATE_PREHEAT;
    }
    break;

  case REFLOW_STATE_PREHEAT:
    // If minimum soak temperature is achieve       
    if (mainInput >= TEMPERATURE_SOAK_MIN)
    {
      // Chop soaking period into smaller sub-period
      timerSoak = millis() + SOAK_MICRO_PERIOD;
      // Set less agressive PID parameters for soaking ramp
      mainOvenPID.SetTunings(PID_KP_SOAK, PID_KI_SOAK, PID_KD_SOAK);
      // Ramp up to first section of soaking temperature
      mainSetpoint = TEMPERATURE_SOAK_MIN + SOAK_TEMPERATURE_STEP;   
      // Proceed to soaking state
      reflowState = REFLOW_STATE_SOAK; 
    }
    break;

  case REFLOW_STATE_SOAK:     
    // If micro soak temperature is achieved       
    if (millis() > timerSoak)
    {
      timerSoak = millis() + SOAK_MICRO_PERIOD;
      // Increment micro setpoint
      mainSetpoint += SOAK_TEMPERATURE_STEP;
      if (mainSetpoint > TEMPERATURE_SOAK_MAX)
      {
        // Set agressive PID parameters for reflow ramp
        mainOvenPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
        // Ramp up to first section of soaking temperature
        mainSetpoint = TEMPERATURE_REFLOW_MAX;   
        // Proceed to reflowing state
        reflowState = REFLOW_STATE_REFLOW; 
      }
    }
    break; 

  case REFLOW_STATE_REFLOW:
    // We need to avoid hovering at peak temperature for too long
    // Crude method that works like a charm and safe for the components
    if (mainInput >= (TEMPERATURE_REFLOW_MAX - 5))
    {
      // Set PID parameters for cooling ramp
      mainOvenPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
      // Ramp down to minimum cooling temperature
      mainSetpoint = TEMPERATURE_COOL_MIN;   
      // Proceed to cooling state
      reflowState = REFLOW_STATE_COOL; 
    }
    break;

  case REFLOW_STATE_COOL:
    // If minimum cool temperature is achieve       
    if (mainInput <= TEMPERATURE_COOL_MIN)
      reflowState = REFLOW_STATE_COMPLETE; 
    break;    

  case REFLOW_STATE_COMPLETE:
      // Turn off buzzer and green LED
      {
        // do something here 
      }
      // Reflow process ended
      reflowState = REFLOW_STATE_IDLE; 
    break;
  
  case REFLOW_STATE_TOO_HOT:
    // If oven temperature drops below room temperature
    if (mainInput < TEMPERATURE_ROOM)
      reflowState = REFLOW_STATE_IDLE;
    break;
    
  case REFLOW_STATE_ERROR:
    // If thermocouple problem is still present
    if (isnan(mainInput) || isnan(auxiInput) )
    {
      // Wait until thermocouple wire is connected
      goto show_message;
    }
    else
      // Clear to perform reflow process
      reflowState = REFLOW_STATE_IDLE; 
    break;    
  }    

  // PID computation and SSR control
  timeNow = millis();

  mainOvenPID.Compute();

  if((timeNow - windowStartTime) > PID_WINDOW_SIZE)
  { 
    // Time to shift the Relay Window
    windowStartTime += PID_WINDOW_SIZE;
  }

  if(mainOutput > (timeNow - windowStartTime))
  {
//      digitalWrite(ssrPin, HIGH);
  }
  else
  {
//      digitalWrite(ssrPin, LOW);   
  }

show_message:
 
  u8g.firstPage();
  do {
    dtostrf(mainInput, 4, 2, strThermoCelsius); 
    u8g.drawStr(0, fontHeight*2+1, strThermo1Text); u8g.drawStr(u8g.getStrWidth(strThermo1Text), fontHeight*2+1, strThermoCelsius);
    dtostrf(auxiInput, 4, 2, strThermoCelsius); 
    u8g.drawStr(0, fontHeight*3+1, strThermo2Text); u8g.drawStr(u8g.getStrWidth(strThermo2Text), fontHeight*3+1, strThermoCelsius);
    dtostrf(mainOutput, 4, 0, strPidOutputBuf); 
    u8g.drawStr(0, fontHeight*1+1, "PID:"); u8g.drawStr(u8g.getStrWidth("PID:"), fontHeight*1+1, strPidOutputBuf);

    if ( reflowState == REFLOW_STATE_IDLE )
      u8g.drawStr(0, fontHeight*0+1, "Ready!");
    else if ( reflowState == REFLOW_STATE_TOO_HOT )
      u8g.drawStr(0, fontHeight*0+1, "Wait! hot!");
    else if ( reflowState == REFLOW_STATE_PREHEAT )
      u8g.drawStr(0, fontHeight*0+1, "Preheating...");
    else if ( reflowState == REFLOW_STATE_SOAK )
      u8g.drawStr(0, fontHeight*0+1, "Soaking...");
    else if ( reflowState == REFLOW_STATE_REFLOW )
      u8g.drawStr(0, fontHeight*0+1, "Reflowing ..."); 
    else if ( reflowState == REFLOW_STATE_COMPLETE )
      u8g.drawStr(0, fontHeight*0+1, "Completed !");
    else
      u8g.drawStr(0, fontHeight*0+1, "Error !");    
  }
  while( u8g.nextPage() );

  delay(500);
    
}
