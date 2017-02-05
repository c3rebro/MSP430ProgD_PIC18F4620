/*==========================================================================*\ 
|                                                                            | 
| LowLevelFunc.h                                                             | 
|                                                                            | 
| Low Level function prototypes, macros, and pin-to-signal assignments       | 
| regarding to user's hardware                                               | 
|----------------------------------------------------------------------------| 
| Project:              MSP430 Replicator                                    | 
| Developed using:      IAR Embedded Workbench 3.40B [Kickstart]             | 
|----------------------------------------------------------------------------| 
| Author:               FRGR                                                 | 
| Version:              1.5                                                  | 
| Initial Version:      04-17-02                                             | 
| Last Change:          20-12-05                                             | 
|----------------------------------------------------------------------------| 
| Version history:                                                           | 
| 1.0 04/02 FRGR        Initial version.                                     | 
| 1.1 04/02 FRGR        Included SPI mode to speed up shifting function by 2.| 
|                       (JTAG control now on Port5)                          | 
| 1.2 06/02 ALB2        Formatting changes, added comments. Removed code used| 
|                       for debug purposes during development.               | 
| 1.3 08/02 ALB2        Initial code release with Lit# SLAA149.              | 
| 1.4 09/05 SUN1        Software delays redesigned to use TimerA harware;    | 
|                       see MsDelay() routine. Added TA constant.            | 
| 1.5 12/05 STO         Added RESET pin definition                           | 
|----------------------------------------------------------------------------| 
| Designed 2002 by Texas Instruments Germany                                 | 
\*==========================================================================*/ 
 
/****************************************************************************/ 
/* Macros and Pin-to-Signal assignments which have to be programmed         */ 
/* by the user. This implementation assumes use of an MSP430F149 as the host*/ 
/* controller and the corresponding hardware given in the application       */ 
/* report TBD Appendix A.                                                   */ 
/*                                                                          */ 
/* The following MSP430 example acts as a hint of how to generally          */ 
/* implement a micro-controller programmer solution for the MSP430 flash-   */ 
/* based devices.                                                           */ 
/****************************************************************************/ 
 
#include <msp430x14x.h> 
#include "JTAGfunc.h" 
 
#ifndef __BYTEWORD__ 
#define __BYTEWORD__ 
typedef unsigned int   word; 
typedef unsigned char   byte; 
#endif 
 
// Constants for runoff status 
#define STATUS_ERROR     0      // false 
#define STATUS_OK        1      // true 
#define STATUS_FUSEBLOWN 2      // GetDevice returns if the security fuse is blown 
 
#define STATUS_ACTIVE    2 
#define STATUS_IDLE      3 
 
/****************************************************************************/ 
/* Define section for user, related to the controller used (here MSP430F149)*/ 
/****************************************************************************/ 
 
#define FREQUENCY   8000   // CPU frequency in kHz 
 
// Comment the following two definitons to disable SPI communication mode and 
// enable the bit-bang implementation. 
#define   SPI_MODE        // Host 'F149's SPI port used to shift data to 
                          // target. (~2x speed increase over a bit- 
                          // bang method using GPIO pins.) 
#define   SPI_DIV      2  // SPI clock division factor (min. = 2) 
 
//---------------------------------------------------------------------------- 
// Pin-to-Signal Assignments 
//---------------------------------------------------------------------------- 
 
// Constants for JTAG and supply control port pins: 
#define JTAGIN       P5IN   // Control ports are on P5.x 
#define JTAGOUT      P5OUT 
#define JTAGDIR      P5DIR 
#define JTAGSEL      P5SEL 
#define TMS          0x01   // P5.0 JTAG TMS input pin 
#define TDI          0x02   // P5.1 JTAG TDI input pin  (SIMO1 if SPI mode) 
#define TDO          0x04   // P5.2 JTAG TDO output pin (SOMI1 if SPI mode) 
#define TCK          0x08   // P5.3 JTAG TCK input pin  (UCLK1 if SPI mode) 
#define TDICTRL2     0x10   // P5.4 switch TDO to TDI 
#define TDICTRL1     0x20   // P5.5 connects TDI 
#define TEST         0x40   // P5.6 TEST pin (20 & 28-pin devices only) 
#define VCCTGT       0x80   // P5.7 Supply voltage of target board 
#define TCLK         TDI    // P5.1 TDI (former XOUT) receives TCLK 
 
// Constants for SPI (directed to target device JTAG port) 
#define UTXBUF      U1TXBUF // SPI1 used 
#define URXBUF      U1RXBUF // SPI1 used 
#define UBR0        UBR01   // SPI1 used 
#define UBR1        UBR11   // SPI1 used 
#define UMCTL       UMCTL1  // SPI1 used 
#define UTCTL       UTCTL1  // SPI1 used 
#define UCTL        UCTL1   // SPI1 used 
#define ME          ME2     // SPI1 used: Module Enable 
#define USPIE       USPIE1  // SPI1 used: Module Enable 
 
// Constants for Error LED control port: 
#define LEDOUT      P1OUT   // LED ports are P1.x 
#define LEDDIR      P1DIR 
#define LEDSEL      P1SEL 
#define LEDRED      0x40    // P1.6 Red LED (ERROR) 
#define LEDGREEN    0x80    // P1.7 Green LED (OK) 
 
// Constants for VPP (Fuse blowing voltage) control port: 
#define VPPOUT      P3OUT   // VPP ports are P3.x 
#define VPPDIR      P3DIR 
#define VPPSEL      P3SEL 
#define VPPONTEST   0x01    // P3.0 Fuse voltage switched to TEST 
#define VPPONTDI    0x02    // P3.1 Fuse voltage switched to TDI 
#define RESET       0x04    // P3.2 Reset pin 
 
// Constants for Setting up Timer A 
#define ONEMS	0x03E8	    // CCR0 delay for 1ms with a 1MHz TA clock 
 
/*---------------------------------------------------------------------------- 
   Macros for processing the JTAG port and Vpp pins 
*/ 
#define ClrTMS()         ((JTAGOUT) &= (~TMS)) 
#define SetTMS()         ((JTAGOUT) |= (TMS)) 
#define ClrTDI()         ((JTAGOUT) &= (~TDI)) 
#define SetTDI()         ((JTAGOUT) |= (TDI)) 
#define ClrTCK()         ((JTAGOUT) &= (~TCK)) 
#define SetTCK()         ((JTAGOUT) |= (TCK)) 
#define ClrTCLK()        ((JTAGOUT) &= (~TCLK)) 
#define SetTCLK()        ((JTAGOUT) |= (TCLK)) 
#define StoreTCLK()      ((JTAGOUT  &   TCLK)) 
#define RestoreTCLK(x)   (x == 0 ? (JTAGOUT &= ~TCLK) : (JTAGOUT |= TCLK)) 
#define ScanTDO()        ((JTAGIN   &   TDO))   // assumes TDO to be bit0 
#define VPPon(x)         (x == VPP_ON_TEST ? (VPPOUT |= VPPONTEST) : (VPPOUT |= VPPONTDI)) 
#define VPPoff()         ((VPPOUT)  &= (~(VPPONTDI | VPPONTEST))) 
 
#define SetRST()          VPPOUT  |=  RESET; //VPPDIR  |=  RESET 
#define ClrRST()          VPPOUT  &= ~RESET; //VPPDIR  &= ~RESET 
#define ReleaseRST()     (VPPDIR  &= ~RESET) 
#define SetTST()         (JTAGOUT |=  TEST) 
#define ClrTST()         (JTAGOUT &= ~TEST) 
 
/*---------------------------------------------------------------------------- 
   Low Level function prototypes 
*/ 
void MsDelay(word milliseconds);      // millisecond delay loop, uses Timer_A 
void usDelay(word microeconds);       // microsecond delay loop, uses nops 
void InitController(void); 
void InitTarget(void); 
void ReleaseTarget(void); 
word Shift(word Format, word Data);   // used for IR- as well as DR-shift 
void TDOisInput(void); 
void TCLKstrobes(word Amount); 
void ShowStatus(word Status, word Index); 
void TriggerPulse(word Mode);         // optional for test 

