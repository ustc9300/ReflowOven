/* LCD  Arduino
   PIN1      = GND
   PIN2      = 5V
   RS(CS)    = 8; 
   RW(SID)   = 9; 
   EN(CLK)   = 3;
   PIN15 PSB = GND;
*/

/* backlight 
   PIN19     = VCC 
   PIN20     = GND
   PIN3 untested
*/

// http://www.geek-workshop.com/thread-91-1-1.html

#include "LCD12864RSPI.h"

#define AR_SIZE( a ) sizeof( a ) / sizeof( a[0] )
unsigned char show0[]="GE YONG"; 
unsigned char show1[]= { 0xB9, 0xA4, 0xD7, 0xF7, 0xCA, 0xD2 };   //工作室
unsigned char show2[]="Touch You Future"; //Touch You Future
 
void setup(){
  LCDA.Initialise(); // 屏幕初始化
  delay(100);
}
 
void loop(){  

  LCDA.CLEAR();//清屏
  delay(1000);
  
  LCDA.CLEAR();//清屏
  delay(100);
  
  LCDA.DisplayString(0,0,show0,AR_SIZE(show0));//第一行第一格开始，显示"YFROBOT"
  delay(100);
  
  LCDA.DisplayString(0,4,show1,AR_SIZE(show1));//第一行第五个字开始，显示文字"工作室"
  delay(100);
  
  LCDA.DisplayString(2,0,show2,AR_SIZE(show2));//第三行第一格开始，显示"Touch You Future"
  delay(1000);
}
