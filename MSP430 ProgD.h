/*==========================================================================*\ 
|                                                                            | 
| MSP430 Programming Device (MSP430 ProgD)                                   | 
|                                                                            | 
| This is the headerfile for the main program codefile and contains:         | 
|                                                                            | 
| - function prototypes                                                      | 
| - CONFIG1 and CONFIG2 for Microchip PIC16f887                              | 
| - constants and macros                                                     | 
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

//#include <p18f4620.h>

byte getFirmwareVersion(void);
byte cacheFirmware(byte);
byte drawFirmwareInfo(byte fwVer);
byte writeFirmware(word startAdr, word length);
void drawInputSgmt(byte firmwareVersion);
void drawOutputSgmt(byte firmwareVersion);
void drawNetID(byte firmwareVersion);
void drawFreq(byte firmwareVersion);
void rxInISR(void);
byte ascii2hex(byte);
byte hex2ascii(byte);

#define     NOT_KNOWN   0u
#define     INT_LN_R    0x01u
#define     RN_ER_2009  0x02u
#define     RN_ER_2012  0x03u

#define     NETID       0x00u
#define     SEGM_IN     0x01u
#define     SEGM_OUT    0x02u
#define     FREQ_A      0x03u
#define		FREQ_B		0x04u

#define     INT_LN_R_NETID      0x8010u
#define     INT_LN_R_FREQ_A     0xA0E2u
#define		INT_LN_R_FREQ_B		0xA0E4u
#define     INT_LN_R_SGMT_IN    0x8012u

#define     RN_ER_2009_NETID    0x8010u
#define     RN_ER_2009_FREQ_A   0x8012u
#define     RN_ER_2009_FREQ_B   0x8012u
#define     RN_ER_2009_SGMT_IN  0x8012u
#define     RN_ER_2009_SGMT_OUT 0x8024u

#define     RN_ER_2012_NETID    0x8010u
#define     RN_ER_2012_FREQ     0x8012u
#define     RN_ER_2012_SGMT_IN  0x8012u
#define     RN_ER_2012_SGMT_OUT 0x8024u

#define     FREQ_868050KHZ      0x0021u
#define     FREQ_868297KHZ      0x0021u
#define     FREQ_869            0x0042u
