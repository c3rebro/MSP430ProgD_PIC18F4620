/*==========================================================================*\ 
|                                                                            | 
| LowLevelFunc.h                                                             | 
|                                                                            | 
| Low Level function prototypes, macros, and pin-to-signal assignments       | 
| regarding to user's hardware. This file has been modified.                 |
|                                                                            | 
|                                                                            |  
|----------------------------------------------------------------------------| 
| Project:              MSP430 ProgD                                         | 
| Developed using:      Microchip MPLAB IDE v8.66 + HITECHC Toolchain 9.83   |
|----------------------------------------------------------------------------| 
| Author:               FRGR + STR                                           | 
| Version:              1.0                                                  | 
| Initial Version:      04-17-02                                             | 
| Last Change:          08-18-12                                             | 
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
| 1.6 08/12 STR         - Changed definitions for new host CPU PIC16F887:    |
|                       - TMR1 Interrupt for MsDelay(word)                   |
|                       - new pin-to-signal assignments and macros
|----------------------------------------------------------------------------| 
| Designed 2002 by Texas Instruments Germany                                 | 
| modified 2012 by Steven Rott Germany                                       | 
\*==========================================================================*/ 
 
// #include <p18f4620.h>
 
#ifndef __BYTEWORD__ 
#define __BYTEWORD__ 
typedef unsigned int   	word; 
typedef unsigned char   byte;
typedef unsigned long	dword;
#endif 
 

/*---------------------------------------------------------------------------- 
   Low Level function prototypes 
*/ 
void msDelay(word milliseconds);      // millisecond delay loop, uses Timer_A 
void usDelay(word microseconds);       // microsecond delay loop, uses nops 
void initController(void); 
void initTarget(void); 
void releaseTarget(void); 
word shiftOut(word Format, word Data);   // used for IR- as well as DR-shift 
void tdoIsInput(void); 
void tcklStrobes(word Amount); 
void showStatus(word Status, word Index); 
void triggerPulse(word Mode);

// Constants for runoff and status 
#define STATUS_ERROR     0      // false 
#define STATUS_OK        1      // true 
#define STATUS_FUSEBLOWN 2      // GetDevice returns if the security fuse is blown 

#define TRUE             1
#define FALSE            0

#define STATUS_ACTIVE    2 
#define STATUS_IDLE      3 
 
/****************************************************************************/ 
/* Define section for user, related to the controller used (here PIC16f887) */ 
/****************************************************************************/ 
 
#define FREQUENCY   8000   // CPU frequency in kHz 
 
// Comment the following two definitons to disable SPI communication mode and 
// enable the bit-bang implementation. 
// #define   SPI_MODE        // Host 'F149's SPI port used to shift data to 
                          // target. (~2x speed increase over a bit- 
                          // bang method using GPIO pins.) 
// #define   SPI_DIV      2  // SPI clock division factor (min. = 2) 
 
//---------------------------------------------------------------------------- 
// Pin-to-Signal Assignments 
//---------------------------------------------------------------------------- 

// Constants for Setting up Timer 1 (TMR1)

#define CIFG		0x01;
#define STMR1		0x01; 

// Constants for Display Controlport and Keyboard pins

#define DISPCP		LATA

#define RD          0x02   // PORTA.1 JTAG TMS input pin 
#define WR          0x01   // PORTA.0 JTAG TDI input pin  (SIMO1 if SPI mode) 
#define CD          0x08   // PORTA.3 JTAG TDO output pin (SOMI1 if SPI mode) 
#define CS          0x04   // PORTA.2 JTAG TCK input pin  (UCLK1 if SPI mode) 

#define SRAMSEL		0x80   // PORTA.7 SRAM Chip Enable
#define AEN			0x40   // PORTA.6 SRAM Adress Enable
#define LAL			0x20   // PORTA.5 LOWER Adress Latch (A0..A7)
#define HAL			0x10   // PORTA.4 HIGHER Adress Latch (A9..A15)

// Constants for JTAG and supply control port pins:  
#define JTAGPORT     PORTC //P5OUT 
#define JTAGDIR      TRISC //P5DIR 
#define TMS          0x04   // PC.0 JTAG TMS input pin 
#define TDI          0x02   // PC.1 JTAG TDI input pin  (SIMO1 if SPI mode) 
#define TDO          0x01   // PC.2 JTAG TDO output pin (SOMI1 if SPI mode) 
#define TCK          0x08   // PC.3 JTAG TCK input pin  (UCLK1 if SPI mode) 
#define TDICTRL2     0x20   // PC.4 switch TDO to TDI 
#define RESET        0x10   // PC.5 Target RESET 
#define TDICTRL1     0x40   // P5.6 TEST pin (20 & 28-pin devices only) 
#define VCCTGT       0x80   // PC.7 Supply voltage of target board 
#define TCLK         TDI    // PC.1 TDI (former XOUT) receives TCLK 
 
// Constants for SPI (directed to target device JTAG port) 
/*
#define UTXBUF      U1TXBUF // SPI1 used 
#define URXBUF      U1RXBUF // SPI1 used 
#define UBR0        UBR01   // SPI1 used 
#define UBR1        UBR11   // SPI1 used 
#define UMCTL       UMCTL1  // SPI1 used 
#define UTCTL       UTCTL1  // SPI1 used 
#define UCTL        UCTL1   // SPI1 used 
#define ME          ME2     // SPI1 used: Module Enable 
#define USPIE       USPIE1  // SPI1 used: Module Enable 
*/

/* 
// Constants for Error LED control port: 
#define LEDOUT      P1OUT   // LED ports are P1.x 
#define LEDDIR      P1DIR 
#define LEDSEL      P1SEL 
#define LEDRED      0x40    // P1.6 Red LED (ERROR) 
#define LEDGREEN    0x80    // P1.7 Green LED (OK) 
*/

/* 
// Constants for VPP (Fuse blowing voltage) control port: 
#define VPPOUT      PORTB   // VPP ports are P3.x 
#define VPPDIR      PORTB 
#define VPPSEL      PORTB 
#define VPPONTEST   0x01    // P3.0 Fuse voltage switched to TEST 
#define VPPONTDI    0x02    // P3.1 Fuse voltage switched to TDI 
*/

 
/*---------------------------------------------------------------------------- 
   Macros for processing the JTAG and the DISPLAY port and Vpp pins 
*/ 

#define ClrRD()			((DISPCP) &= (~RD))
#define SetRD()			((DISPCP) |= (RD))
#define ClrWR()			((DISPCP) &= (~WR))
#define SetWR()			((DISPCP) |= (WR))
#define ClrCD()			((DISPCP) &= (~CD))
#define SetCD()			((DISPCP) |= (CD))
#define ClrCS()			((DISPCP) &= (~CS))
#define SetCS()			((DISPCP) |= (CS))

#define ClrSRAMSEL()	((DISPCP) &= (~SRAMSEL))
#define SetSRAMSEL()	((DISPCP) |= (SRAMSEL))
#define ClrAEN()		((DISPCP) &= (~AEN))
#define SetAEN()		((DISPCP) |= (AEN))
#define ClrLAL()		((DISPCP) &= (~LAL))
#define SetLAL()		((DISPCP) |= (LAL))
#define ClrHAL()		((DISPCP) &= (~HAL))
#define SetHAL()		((DISPCP) |= (HAL))

#define ClrTMS()         ((JTAGPORT) &= (~TMS)) 
#define SetTMS()         ((JTAGPORT) |= (TMS)) 
#define ClrTDI()         ((JTAGPORT) &= (~TDI)) 
#define SetTDI()         ((JTAGPORT) |= (TDI)) 
#define ClrTCK()         ((JTAGPORT) &= (~TCK)) 
#define SetTCK()         ((JTAGPORT) |= (TCK)) 
#define ClrTCLK()        ((JTAGPORT) &= (~TCLK)) 
#define SetTCLK()        ((JTAGPORT) |= (TCLK))
#define ClrRST()         ((JTAGPORT) &= (~RESET))
#define SetRST()         ((JTAGPORT) |= (RESET))

#define StoreTCLK()      ((JTAGPORT  &   TCLK)) 
#define RestoreTCLK(x)   (x == 0u ? (JTAGPORT &= ~TCLK) : (JTAGPORT |= TCLK)) 

#define ScanTDO()        ((JTAGPORT   &   TDO))   // assumes TDO to be bit0 
#define VPPon(x)         (x == VPP_ON_TEST ? (VPPOUT |= VPPONTEST) : (VPPOUT |= VPPONTDI)) 
#define VPPoff()         ((VPPOUT)  &= (~(VPPONTDI | VPPONTEST))) 