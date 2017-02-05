/*==========================================================================*\ 
|                                                                            | 
| MSP430 Programming Device (MSP430 ProgD)                                   | 
|                                                                            | 
| This is the headerfile for the IO codefile and contains:                   | 
|                                                                            | 
| - function prototypes                                                      |  
| - constant for the keypad                                                  | 
|                                                                            | 
|----------------------------------------------------------------------------| 
| Project:              MSP430 ProgD                                         | 
| Developed using:      Microchip MPLAB IDE v8.66 + HITECHC Toolchain 9.83   | 
|----------------------------------------------------------------------------| 
| Author:               STR                                                  | 
| Version:              1.0                                                  | 
| Initial Version:                                                           | 
| Last Change:                                                               | 
|----------------------------------------------------------------------------| 
| Version history:                                                           | 
| 1.0 08/12 STR        Initial version.                                      | 
|                                                                            |
|----------------------------------------------------------------------------| 
| Designed 2012 by Steven Rott                                               | 
\*==========================================================================*/ 

#define transl cmd_dat-=0x20			              //subtract 0x20 from ascii code due to shift in ascii code 
									              //from T6963C character set
#define HEX		0x00
#define ASCII	0x02

// #include <p18f4620.h>

void displayInit(void);					              //initialisation routine T6963C
void cls(void);
void display(byte,byte,byte,byte);   	              //T6963 communication protocol
void printToScreen(byte, byte*, rom byte*,byte,byte);       //put data to screen
void getDisplayStatus(void);			              	  //T6963C state check
byte waitForSingleKey(byte dataSel);						              //returns a key from 4x4 keypad
void txOut(byte,rom byte*);
word getWholeCommand(void);
void sramWrite(word adress, byte data);
byte sramRead(word adress);
byte hex2ascii(byte);
