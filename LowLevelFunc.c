/*==========================================================================*\  
|                                                                            |  
| LowLevelFunc.c                                                             |  
|                                                                            |  
| Low Level Functions regarding PIC16F887                                    |  
|----------------------------------------------------------------------------|  
| Project:              MSP430 ProgD                                         | 
| Developed using:      Microchip MPLAB IDE v8.66 + HITECHC Toolchain 9.83   |  
|----------------------------------------------------------------------------|  
| Author:               FRGR + STR                                           |  
| Version:              1.6                                                  |  
| Initial Version:      04-17-02                                             |  
| Last Change:          08-18-12                                             |  
|----------------------------------------------------------------------------|  
| Version history:                                                           |  
| 1.0 04/02 FRGR        Initial version.                                     |  
| 1.1 04/02 FRGR        Included SPI mode to speed up shifting function by 2.|  
| 1.2 06/02 ALB2        Formatting changes, added comments.                  |  
| 1.3 08/02 ALB2        Initial code release with Lit# SLAA149.              |  
| 1.4 09/05 SUN1        Software delays redesigned to use TimerA harware;    |  
|                       see msDelay() routine. Added TA setup                |  
| 1.5 12/05 STO         Adapted for 2xx devices with SpyBiWire using 4JTAG   |  
| 1.6 08/12 STR         modified for use with host PIC16F887                 | 
|----------------------------------------------------------------------------|  
| Designed 2002 by Texas Instruments Germany                                 |  
\*==========================================================================*/   
   
#include "LowLevelFunc.h" 
#include "p18f4620.h"
#include "JTagFunc.h"

word iter1=0;
   
/****************************************************************************/   
/* Function declarations which have to be programmed by the user for use    */   
/* with hosts other than the MSP430F149.                                    */   
/*                                                                          */   
/* The following MSP430F149-specific code can be used as a reference as to  */   
/* how to implement the required JTAG communication on additional hosts.    */   
/****************************************************************************/   
   
/*----------------------------------------------------------------------------  
   Initialization of the Controller Board (Master or Host) PIC16F887 
*/   
void initController(void)   
{   
	
	//OPTION_REG=0b10000000; 
	OSCCON=0b11111111;
    RCON=0b00000000; 
	//VRCON=0b00000000;
	//CM2CON=0b00000000;
	//CM1CON=0b00000000;
	T2CON=0b00000000;
	T3CON=0b00000000;
	T1CON=0b00000100;
	//SSPCON=0b00000000;
	ADCON0=0b00000000;
	ADCON1=0b00001111;
	CCP1CON=0b00000000;
	CCP2CON=0b00000000;
	//UART initialization
	SPBRG = 51;             //BAUD19200 = 25 internal OSC w/8MHz
	TXSTA = 0b00100100;     //enable asynchronous mode, enable transmit
	RCSTA = 0b10010000;     //enable serial port, enable continuous rx
	OSCTUNE=0x40;

    PIE1=0b00100000;
	INTCON=0b11000000;
	INTCON2=0b00000000;
	
}   
   
/*----------------------------------------------------------------------------  
   This function switches TDO to Input, used for fuse blowing
   ---> this is not implemented  
*/   
void tdoIsInput(void)   
{   
    JTAGPORT &= ~TDICTRL1;           // Release TDI pin on target   
    msDelay(5);                     // Settle MOS relay   
    JTAGPORT |=  TDICTRL2;           // Switch TDI --> TDO   
    msDelay(5);                     // Settle MOS relay   
}   
   
/*----------------------------------------------------------------------------  
   Initialization of the Target Board (switch voltages on, preset JTAG pins)  
    for devices with normal 4wires JTAG  (JTAG4SBW=0)  
    for devices with Spy-Bi-Wire to work in 4wires JTAG (JTAG4SBW=1)  
*/   
void initTarget(void)   
{     
    TRISD  = TDI | TMS | TCK | TCLK | TDICTRL1 | VCCTGT;   
    JTAGDIR  = TDI | TMS | TCK | TCLK | TDICTRL1 | TDICTRL2 | VCCTGT;   
   
    //VPPSEL  &= ~(VPPONTDI | VPPONTEST | RESET); // No special function, I/Os   
    //VPPOUT  &= ~(VPPONTDI | VPPONTEST);         // VPPs are OFF   
    //VPPOUT |= RESET;                            // Release target reset   
    //VPPDIR  |=  (VPPONTDI | VPPONTEST | RESET); // VPP pins are outputs   
    msDelay(50);                // Settle MOS relays, target capacitor   
}   
      
/*----------------------------------------------------------------------------  
   Release Target Board (switch voltages off, JTAG pins are HI-Z)  
*/   
void releaseTarget(void)   
{   
    //VPPoff();               // VPPs are off (safety)   
    msDelay(5);             // Settle MOS relays   
    JTAGDIR  =  0xFF;        // VCC is off, all I/Os are HI-Z   
    msDelay(5);             // Settle MOS relays   
}   
     
//----------------------------------------------------------------------------   
/*  Shift a value into TDI (MSB first) and simultaneously shift out a value  
    from TDO (MSB first).  
    Note:      When defining SPI_MODE the embedded SPI is used to speed up by 2.  
    Arguments: word Format (number of bits shifted, 8 (F_BYTE) or 16 (F_WORD))  
               word Data (data to be shifted into TDI)  
    Result:    word (scanned TDO value)  
*/ 

word shiftOut(word format, word data)   
{   
    word tclk = StoreTCLK();        // Store TCLK state;   
    word tdoWord = 0x0000;          // Initialize shifted-in word   
    word msb = 0x0000;   

    (format == F_WORD) ? (msb = 0x8000) : (msb = 0x80);
   
    for (iter1 = format; iter1 > 0u; iter1--)   
    {   
        ((data & msb) == 0u) ? ClrTDI() : SetTDI();   
        data <<= 1;   
        if (iter1 == 1u)                 // Last bit requires TMS=1   
           SetTMS();
		//usDelay(2);   
        ClrTCK();
		//usDelay(2);   
        SetTCK();   
        tdoWord <<= 1;                // TDO could be any port pin   
        if (!ScanTDO())   //
            tdoWord|=0x0001;   
    }      
   
    // common exit   
    RestoreTCLK(tclk);              // restore TCLK state   
   	//	usDelay(2);
    // JTAG FSM = Exit-DR   
    ClrTCK();
	//	usDelay(2);   
    SetTCK();
	//	usDelay(2);   
    // JTAG FSM = Update-DR   
    ClrTMS(); 
	//	usDelay(2);  
    ClrTCK();   
	//	usDelay(2);
    SetTCK();   
    // JTAG FSM = Run-Test/Idle   
    return(tdoWord);   
}   
  
/*---------------------------------------------------------------------------  
   Delay function (resolution is 1 ms)  
   Arguments: word millisec (number of ms, max number is 0xFFFF)  
*/   

void msDelay(word milliseconds)   
{   
   word i;   
   for(i = milliseconds; i > 0u; i--)   
   {   
        PIR1bits.TMR1IF &= ~CIFG;          // Clear the interrupt flag
		TMR1H = 0xF8;			  // preload timer = 1 Ms 
		TMR1L = 0x20;  
        T1CONbits.TMR1ON |= STMR1;          // start timer   
        while (!PIR1bits.TMR1IF);          // Wait until the Timer elapses   
        T1CONbits.TMR1ON &= ~STMR1;         // Stop Timer   
    }   
}   
       
/*---------------------------------------------------------------------------  
   Delay function (resolution is ~1 us at 8MHz clock)  
   Arguments: word microeconds (number of ms, max number is 0xFFFF)  
*/   
void usDelay(word microeconds)   
{   
    do   
    {   
        Nop();   
        Nop();   
    }   
    while (--microeconds > 0u);   
}   
   
/*---------------------------------------------------------------------------  
   This function generates Amount strobes with the Flash Timing Generator  
   Frequency fFTG = 257..476kHz (t = 3.9..2.1us).  
   User knows target frequency, instruction cycles, C implementation.  
   Arguments: word Amount (number of strobes to be generated)  
*/   
#define S_LOOPBODY      14      // 14 cycles/loop w/o NOPs   
#define S_ADDNOPS   (word)((FREQUENCY * 2.1) / 1000 - S_LOOPBODY + 1)   
                        // S_ADDNOPS = 3..18   
void tcklStrobes(word amount)   
{   
    volatile word i;   
   
    for (i = amount; i > 0u; i--)     // This implementation has 14 body cycles!   
    {   
        JTAGPORT |=  TCLK;       // Set TCLK   
        Nop();                 // Include NOPs if necessary (min. 3)   
        Nop();   
        Nop();   
        Nop();   
        Nop();   
        Nop();   
        JTAGPORT &= ~TCLK;       // Reset TCLK   
    }   
}   
   
/*----------------------------------------------------------------------------  
   This function controls the status LEDs depending on the status  
   argument. It stops program in error case.  
   Arguments: word status (4 stati possible for 2 LEDs)  
              word index (additional number for detailed diagnostics or  
                          watch variable during debugging phase)  
*/  
/* 
void ShowStatus(word status, word index)   
{   
    LEDOUT  &= ~(LEDGREEN | LEDRED);    // Both LEDs are off   
    switch (status)   
    {   
        case STATUS_ERROR:   
            LEDOUT  |= LEDRED;          // Switch red LED on   
            ReleaseTarget();            // Voltages off, JTAG HI-Z   
            while(index);               // Stop program, index must be > 0   
        case STATUS_ACTIVE:;            // Switch both LEDs on   
            LEDOUT  |= LEDRED;   
        case STATUS_OK:                 // Switch green LED on   
            LEDOUT  |= LEDGREEN;   
        case STATUS_IDLE:;              // Keap both LEDs switched off   
    }   
}                                       // return if active, idle, ok   
*/       
/*----------------------------------------------------------------------------  
   This function performs a Trigger Pulse for test/development  
*/
/*   
#ifdef DEBUG   
void TriggerPulse(word mode)   
{   
    switch (mode)   
    {   
        case 1: LEDOUT  |=  TRIGGER;    // mode = 1: set trigger   
                break;   
        case 2: LEDOUT  |=  TRIGGER;    // mode = 2: set/reset trigger   
        case 0: LEDOUT  &= ~TRIGGER;    // mode = 0: reset trigger   
    }   
}   
#endif   
*/   
/****************************************************************************/   
/*                         END OF SOURCE FILE                               */   
/****************************************************************************/   


