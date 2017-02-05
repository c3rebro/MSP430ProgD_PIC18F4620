/*==========================================================================*\ 
|                                                                            | 
| JTAGfunc.c                                                                 | 
|                                                                            | 
| JTAG Control Sequences for Erasing / Programming / Fuse Burning            | 
|----------------------------------------------------------------------------| 
| Project:              JTAG Functions                                       | 
| Developed using:      IAR Embedded Workbench 3.40B [Kickstart]             | 
|----------------------------------------------------------------------------| 
| Author:               STO                                                  | 
| Version:              1.6                                                  | 
| Initial Version:      04-17-02                                             | 
| Last Change:          07-31-06                                             | 
|----------------------------------------------------------------------------| 
| Version history:                                                           | 
| 1.0 04/02 FRGR        Initial version.                                     | 
| 1.1 04/02 ALB2        Formatting changes, added comments.                  | 
| 1.2 08/02 ALB2        Initial code release with Lit# SLAA149.              | 
| 1.3 09/05 JDI         'ResetTAP': added SetTDI for fuse check              | 
|                       search for F2xxx to find respective modifications in | 
|                       'SetPC', 'HaltCPU', 'VerifyPSA', 'EraseFLASH'        | 
|                       'WriteFLASH'                                         | 
|           SUN1        Software delays redesigned to use TimerA harware;    | 
|                       see MsDelay() routine.                               | 
| 1.4 01/06 STO         Added entry sequence for SpyBiWire devices           | 
|                       Minor cosmetic changes                               | 
| 1.5 03/06 STO         BlowFuse() make correct fuse check after blowing.    | 
| 1.6 07/06 STO         Loop in WriteFLASH() changed.                        | 
|----------------------------------------------------------------------------| 
| Designed 2002 by Texas Instruments Germany                                 | 
\*==========================================================================*/ 
 
#include "JTAGfunc.h" 
#include "LowLevelFunc.h" 
#include "Devices.h" 
 
/****************************************************************************/ 
/* Low level routines for accessing the target device via JTAG:             */ 
/****************************************************************************/ 
 
//---------------------------------------------------------------------------- 
/* Function for shifting a given 16-bit word into the JTAG data register 
   through TDI. 
   Arguments: word data (16-bit data, MSB first) 
   Result:    word (value is shifted out via TDO simultaneously) 
*/ 
word DR_Shift16(word data) 
{ 
    // JTAG FSM state = Run-Test/Idle 
    SetTMS(); 
    ClrTCK(); 
    SetTCK(); 
 
    // JTAG FSM state = Select DR-Scan 
    ClrTMS(); 
    ClrTCK(); 
    SetTCK(); 
    // JTAG FSM state = Capture-DR 
    ClrTCK(); 
    SetTCK(); 
 
    // JTAG FSM state = Shift-DR, Shift in TDI (16-bit) 
    return(Shift(F_WORD, data)); 
    // JTAG FSM state = Run-Test/Idle 
} 
 
//---------------------------------------------------------------------------- 
/* Function for shifting a new instruction into the JTAG instruction 
   register through TDI (MSB first, but with interchanged MSB - LSB, to 
   simply use the same shifting function, Shift(), as used in DR_Shift16). 
   Arguments: byte Instruction (8bit JTAG instruction, MSB first) 
   Result:    word TDOword (value shifted out from TDO = JTAG ID) 
*/ 
word IR_Shift(byte instruction) 
{ 
    // JTAG FSM state = Run-Test/Idle 
    SetTMS(); 
    ClrTCK(); 
    SetTCK(); 
    // JTAG FSM state = Select DR-Scan 
    ClrTCK(); 
    SetTCK(); 
 
    // JTAG FSM state = Select IR-Scan 
    ClrTMS(); 
    ClrTCK(); 
    SetTCK(); 
    // JTAG FSM state = Capture-IR 
    ClrTCK(); 
    SetTCK(); 
 
    // JTAG FSM state = Shift-IR, Shift in TDI (8-bit) 
    return(Shift(F_BYTE, instruction)); 
    // JTAG FSM state = Run-Test/Idle 
} 
 
//---------------------------------------------------------------------------- 
/* Reset target JTAG interface and perform fuse-HW check. 
   Arguments: None 
   Result:    None 
*/ 
void ResetTAP(void) 
{ 
    word i; 
 
    // process TDI first to settle fuse current 
    SetTDI(); 
    SetTMS(); 
    SetTCK(); 
 
    // Now fuse is checked, Reset JTAG FSM 
    for (i = 6; i > 0; i--) 
    { 
        ClrTCK(); 
        SetTCK(); 
    } 
    // JTAG FSM is now in Test-Logic-Reset 
    ClrTCK(); 
    ClrTMS(); 
    SetTCK(); 
    // JTAG FSM is now in Run-Test/IDLE 
 
    // Perform fuse check 
    ClrTMS(); 
    SetTMS(); 
    ClrTMS(); 
    SetTMS(); 
} 
 
//---------------------------------------------------------------------------- 
/* Function to execute a Power-Up Clear (PUC) using JTAG CNTRL SIG register 
   Arguments: None 
   Result:    word (STATUS_OK if JTAG ID is valid, STATUS_ERROR otherwise) 
*/ 
word ExecutePUC(void) 
{ 
    word JtagVersion; 
 
    // Perform Reset 
    IR_Shift(IR_CNTRL_SIG_16BIT); 
    DR_Shift16(0x2C01);                 // Apply Reset 
    DR_Shift16(0x2401);                 // Remove Reset 
    ClrTCLK(); 
    SetTCLK(); 
    ClrTCLK(); 
    SetTCLK(); 
    ClrTCLK(); 
    JtagVersion = IR_Shift(IR_ADDR_CAPTURE); // read JTAG ID, checked at function end 
    SetTCLK(); 
 
    WriteMem(F_WORD, 0x0120, 0x5A80);   // Disable Watchdog on target device 
 
    if (JtagVersion != JTAG_ID) 
    { 
        return(STATUS_ERROR); 
    } 
    return(STATUS_OK); 
} 
 
//---------------------------------------------------------------------------- 
/* Function to set target CPU JTAG FSM into the instruction fetch stat