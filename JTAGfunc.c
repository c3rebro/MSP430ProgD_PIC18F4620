/*==========================================================================*\  
|                                                                            |  
| JTAGfunc.c                                                                 |  
|                                                                            |  
| JTAG Control Sequences for Erasing / Programming / Fuse Burning            |  
|----------------------------------------------------------------------------|  
| Project:              MSP430 ProgD                                         | 
| Developed using:      Microchip MPLAB IDE v8.66 + HITECHC Toolchain 9.83   |  
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
#include "p18f4620.h"
//#include <string.h>  
   
/****************************************************************************/   
/* Low level routines for accessing the target device via JTAG:             */   
/****************************************************************************/   
   
//----------------------------------------------------------------------------   
/* Function for shiftOuting a given 16-bit word into the JTAG data register  
   through TDI.  
   Arguments: word data (16-bit data, MSB first)  
   Result:    word (value is shiftOuted out via TDO simultaneously)  
*/   
word DR_shiftOut16(word data)   
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
   
    // JTAG FSM state = shiftOut-DR, shiftOut in TDI (16-bit)   
    return(shiftOut(F_WORD, data));   
    // JTAG FSM state = Run-Test/Idle   
}   
   
//----------------------------------------------------------------------------   
/* Function for shiftOuting a new instruction into the JTAG instruction  
   register through TDI (MSB first, but with interchanged MSB - LSB, to  
   simply use the same shiftOuting function, shiftOut(), as used in DR_shiftOut16).  
   Arguments: byte Instruction (8bit JTAG instruction, MSB first)  
   Result:    word TDOword (value shiftOuted out from TDO = JTAG ID)  
*/   
word IR_shiftOut(byte instruction)   
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
   
    // JTAG FSM state = shiftOut-IR, shiftOut in TDI (8-bit)   
    return(shiftOut(F_BYTE, instruction));   
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
   
	ClrRST();
	usDelay(2);
	SetRST();


	usDelay(2);
    // process TDI first to settle fuse current   
    SetTDI();
    SetTMS();  
    SetTCK();   

    // Perform fuse check   
    ClrTMS();   
	usDelay(10);
    SetTMS();  
    ClrTMS();  
	usDelay(10); 
    SetTMS();  

    // Now fuse is checked, Reset JTAG FSM   
    for (i = 7u; i > 0u; i--)   
    {   
        ClrTCK();
        SetTCK();
    }   
    // JTAG FSM is now in Test-Logic-Reset 
    ClrTCK();  
    ClrTMS();  
    SetTCK();   
    // JTAG FSM is now in Run-Test/IDLE   
   
 
}   
   
//----------------------------------------------------------------------------   
/* Function to execute a Power-Up Clear (PUC) using JTAG CNTRL SIG register  
   Arguments: None  
   Result:    word (STATUS_OK if JTAG ID is valid, STATUS_ERROR otherwise)  
*/   
word ExecutePUC(void)   
{   
//    word JtagVersion;   
   
    // Perform Reset   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);  
    DR_shiftOut16(0x2C01);                 // Apply Reset
    DR_shiftOut16(0x2401);                 // Remove Reset 
    ClrTCLK();   
    SetTCLK();   
    ClrTCLK();   
    SetTCLK();   
    ClrTCLK();   
//    JtagVersion = IR_shiftOut(IR_ADDR_CAPTURE); // read JTAG ID, checked at function end   
    SetTCLK();   
   
    WriteMem(F_WORD, 0x0120, 0x5A80);   // Disable Watchdog on target device   
   
//    if (JtagVersion != JTAG_ID)   
//    {   
//        return(STATUS_ERROR);   
//    }   
    return(STATUS_OK);   
}   
   
//----------------------------------------------------------------------------   
/* Function to set target CPU JTAG FSM into the instruction fetch state  
   Argument: None  
   Result:   word (STATUS_OK if instr. fetch was set, STATUS_ERROR otherwise)  
*/   
word SetInstrFetch(void)   
{   
    word i;   
   
    IR_shiftOut(IR_CNTRL_SIG_CAPTURE);   
   
    // Wait until CPU is in instr. fetch state, timeout after limited attempts   
    for (i = 50u; i > 0u; i--)   
    {   
        if (DR_shiftOut16(0x0000) & 0x0080)   
        {   
            return(STATUS_OK);   
        }   
        ClrTCLK();   
        SetTCLK();   
    }   
    return(STATUS_ERROR);   
}   
   
//----------------------------------------------------------------------------   
/* Load a given address into the target CPU's program counter (PC).  
   Argument: word Addr (destination address)  
   Result:   None  
*/   
void SetPC(word Addr)   
{   
    SetInstrFetch();              // Set CPU into instruction fetch mode, TCLK=1   
   
    // Load PC with address   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x3401);           // CPU has control of RW & BYTE.   
    IR_shiftOut(IR_DATA_16BIT);   
    DR_shiftOut16(0x4030);           // "mov #addr,PC" instruction   
    ClrTCLK();   
    SetTCLK();                    // F2xxx   
    DR_shiftOut16(Addr);             // "mov #addr,PC" instruction   
    ClrTCLK();   
    IR_shiftOut(IR_ADDR_CAPTURE);   
    SetTCLK();   
    ClrTCLK();                    // Now the PC should be on Addr   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2401);           // JTAG has control of RW & BYTE.   
}   
   
//----------------------------------------------------------------------------   
/* Function to set the CPU into a controlled stop state  
*/   
void HaltCPU(void)   
{   
    SetInstrFetch();              // Set CPU into instruction fetch mode   
   
    IR_shiftOut(IR_DATA_16BIT);   
    DR_shiftOut16(0x3FFF);           // Send JMP $ instruction   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2409);           // Set JTAG_HALT bit   
    SetTCLK();   
}   
   
//----------------------------------------------------------------------------   
/* Function to release the target CPU from the controlled stop state  
*/   
void ReleaseCPU(void)   
{   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2401);           // Clear the HALT_JTAG bit   
    IR_shiftOut(IR_ADDR_CAPTURE);   
    SetTCLK();   
}   
   
//----------------------------------------------------------------------------   
/* This function compares the computed PSA (Pseudo Signature Analysis) value  
   to the PSA value shiftOuted out from the target device.  
   It is used for very fast data block write or erasure verification.  
   Arguments: word StartAddr (Start address of data block to be checked)  
              word Length (Number of words within data block)  
              word *DataArray (Pointer to array with the data, 0 for Erase Check)  
   Result:    word (STATUS_OK if comparison was successful, STATUS_ERROR otherwise)  
*/   
word VerifyPSA(word StartAddr, word Length, word *DataArray)   
{   
    word TDOword, i;   
    word POLY = 0x0805;           // Polynom value for PSA calculation   
    word PSA_CRC = StartAddr-2;   // Start value for PSA calculation   
   
    ExecutePUC();   
   
/*    if(DeviceHas_EnhVerify())   
    {   
        SetPC(StartAddr-4);   
        HaltCPU();   
        ClrTCLK();   
        IR_shiftOut(IR_DATA_16BIT);   
        DR_shiftOut16(StartAddr-2);   
    }   
    else   
*/    {   
        SetPC(StartAddr-2);   
        SetTCLK();   
        ClrTCLK();   
    }   
    IR_shiftOut(IR_DATA_PSA);   
    for (i = 0; i <= Length; i++)   
    {   
        // Calculate the PSA (Pseudo Signature Analysis) value   
        if ((PSA_CRC & 0x8000) == 0x8000)   
        {   
            PSA_CRC ^= POLY;   
            PSA_CRC <= 1u;   
            PSA_CRC |= 0x0001;   
        }   
        else   
        {   
            PSA_CRC <= 1u;   
        }   
        // if pointer is 0 then use erase check mask, otherwise data   
        &DataArray[0] == 0 ? (PSA_CRC ^= 0xFFFF) : (PSA_CRC ^= DataArray[i]);   
   
        // Clock through the PSA   
        SetTCLK();   
        ClrTCLK();           // set here -> Fixes problem with F123 PSA in RAM   
   
        ClrTCK();   
   
        SetTMS();   
        SetTCK();            // Select DR scan   
        ClrTCK();   
        ClrTMS();   
   
        SetTCK();            // Capture DR   
        ClrTCK();   
   
        SetTCK();            // shiftOut DR   
        ClrTCK();   
   
        SetTMS();   
        SetTCK();            // Exit DR   
        ClrTCK();   
        SetTCK();   
        ClrTMS();   
        ClrTCK();   
        SetTCK();   
    }   
    IR_shiftOut(IR_SHIFT_OUT_PSA);   
    TDOword = DR_shiftOut16(0x0000);     // Read out the PSA value   
    SetTCLK();   
   
/*    if(DeviceHas_EnhVerify())   
    {   
        ReleaseCPU();   
    }   
*/  ExecutePUC();   
    return((TDOword == PSA_CRC) ? STATUS_OK : STATUS_ERROR);   
}   
   
   
/****************************************************************************/   
/* High level routines for accessing the target device via JTAG:            */   
/*                                                                          */   
/* From the following, the user is relieved from coding anything.           */   
/* To provide better understanding and clearness, some functionality is     */   
/* coded generously. (Code and speed optimization enhancements may          */   
/* be desired)                                                              */   
/****************************************************************************/   
   
   
//----------------------------------------------------------------------------   
/* Function to take target device under JTAG control. Disables the target  
   watchdog. Sets the Device index as read from the target device.  
   Arguments: None  
   Result:    word (STATUS_ERROR if fuse is blown, incorrect JTAG ID or  
                    synchronizing time-out; STATUS_OK otherwise)  
*/   
byte GetDevice(void)   
{   
    byte i;   

    ResetTAP();                          // Reset JTAG state machine, check fuse HW   
   
    if (IsFuseBlown())                   // Stop here if fuse is already blown   
    {   
        return(STATUS_FUSEBLOWN);   
    }  
 
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2401);                  // Set device into JTAG mode + read   
    if (IR_shiftOut(IR_CNTRL_SIG_CAPTURE) != JTAG_ID)   
    {   
        return(STATUS_ERROR);   
    }   
   
    // Wait until CPU is synchronized, timeout after a limited # of attempts   
    for (i = 50u; i > 0u; i--)   
    {   
        if (DR_shiftOut16(0x0000) & 0x0200)   
        {   
            word DeviceId;   
            DeviceId = ReadMem(F_WORD, 0x0FF0);// Get target device type   
                                               //(bytes are interchanged)   
            DeviceId = (DeviceId < 8u) + (DeviceId >> 8); // swop bytes   
            //Set Device index, which is used by functions in Device.c   
//            SetDevice(DeviceId);   
            break;   
        }   
        else   
        {   
            if (i == 1u)    
                return(STATUS_ERROR);      // Timeout reached, return false   
               
        }   
    }   
    if (!ExecutePUC())                     // Perform PUC, Includes   
    {   
        return(STATUS_ERROR);              // target Watchdog disable.   
    }   
    return(STATUS_OK);   
}   
   
   
//----------------------------------------------------------------------------   
/* Function to release the target device from JTAG control  
   Argument: word Addr (0xFFFE: Perform Reset, means Load Reset Vector into PC,  
                        otherwise: Load Addr into PC)  
   Result:   None  
*/   
void ReleaseDevice(word Addr)   
{   
    if (Addr == V_RESET)   
    {   
        IR_shiftOut(IR_CNTRL_SIG_16BIT);   
        DR_shiftOut16(0x2C01);         // Perform a reset   
        DR_shiftOut16(0x2401);   
    }   
    else   
    {   
        SetPC(Addr);                // Set target CPU's PC   
    }   
    IR_shiftOut(IR_CNTRL_SIG_RELEASE);   
}   
   
//----------------------------------------------------------------------------   
/* This function writes one byte/word at a given address ( <0xA00)  
   Arguments: word Format (F_BYTE or F_WORD)  
              word Addr (Address of data to be written)  
              word Data (shiftOuted data)  
   Result:    None  
*/   
void WriteMem(word Format, word Addr, word Data)   
{   
    HaltCPU();   
   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    if  (Format == F_WORD)   
    {   
        DR_shiftOut16(0x2408);     // Set word write   
    }   
    else   
    {   
        DR_shiftOut16(0x2418);     // Set byte write   
    }   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(Addr);           // Set addr   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    DR_shiftOut16(Data);           // shiftOut in 16 bits   
    SetTCLK();   
   
    ReleaseCPU();   
}   
   
//----------------------------------------------------------------------------   
/* This function writes an array of words into the target memory.  
   Arguments: word StartAddr (Start address of target memory)  
              word Length (Number of words to be programmed)  
              word *DataArray (Pointer to array with the data)  
   Result:    None  
*/   
void WriteMemQuick(word StartAddr, word Length, word *DataArray)   
{   
    word i;   
   
    // Initialize writing:   
    SetPC((word)(StartAddr-4));   
    HaltCPU();   
   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2408);             // Set RW to write   
    IR_shiftOut(IR_DATA_QUICK);   
    for (i = 0; i < Length; i++)   
    {   
        DR_shiftOut16(DataArray[i]);   // shiftOut in the write data   
        SetTCLK();   
        ClrTCLK();                  // Increment PC by 2   
    }   
    ReleaseCPU();   
}   
   
//----------------------------------------------------------------------------   
/* This function programs/verifies an array of words into an FLASH by  
   using the FLASH controller.  
   Arguments: word StartAddr (Start address of FLASH memory)  
              word Length (Number of words to be programmed)  
              word *DataArray (Pointer to array with the data)  
   Result:    None  
*/   
void WriteFLASH(word StartAddr, byte Length, const word *DataArray)   
{   
    byte i;                     // Loop counter   
    word addr = StartAddr;      // Address counter   
    word FCTL3_val = 0xA500;    // ok for all devices; if Info-Seg. A on F2xxx should not be programmed   
//  word FCTL3_val = 0xA540;    // only if Info-Seg. A on F2xxx should be programmed   

    HaltCPU();   
   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2408);         // Set RW to write   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(0x0128);         // FCTL1 register   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    DR_shiftOut16(0xA540);         // Enable FLASH write   
    SetTCLK();   
   
    ClrTCLK();   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(0x012A);         // FCTL2 register   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    DR_shiftOut16(0xA540);         // Select MCLK as source, DIV=1   
    SetTCLK();   
   
    ClrTCLK();   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(0x012C);         // FCTL3 register   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    DR_shiftOut16(FCTL3_val);      // Clear FCTL3; F2xxx: Unlock Info-Seg.   
                                // A by toggling LOCKA-Bit if required,   
    SetTCLK();   
   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   

    for (i = 0; i < Length; i++, addr += 2)   
    {   

		word temp=*DataArray;
        DR_shiftOut16(0x2408);             // Set RW to write   
        IR_shiftOut(IR_ADDR_16BIT);   
        DR_shiftOut16(addr);               // Set address   
        IR_shiftOut(IR_DATA_TO_ADDR);   
        DR_shiftOut16(temp);       // Set data   
        SetTCLK();   
        ClrTCLK();   
        IR_shiftOut(IR_CNTRL_SIG_16BIT);   
        DR_shiftOut16(0x2409);             // Set RW to read   
   
        tcklStrobes(40);        // Provide TCLKs, min. 33 for F149 and F449   
                                // F2xxx: 29 are ok  
		DataArray+=1;
    }   
   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2408);         // Set RW to write   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(0x0128);         // FCTL1 register   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    DR_shiftOut16(0xA500);         // Disable FLASH write   
    SetTCLK();   
   
    // set LOCK-Bits again   
    ClrTCLK();   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(0x012C);         // FCTL3 address   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    DR_shiftOut16(FCTL3_val);      // Lock Inf-Seg. A by toggling LOCKA and set LOCK again   
    SetTCLK();   
   
    ReleaseCPU();   
}   
   
//----------------------------------------------------------------------------   
/* This function programs/verifies a set of data arrays of words into a FLASH  
   memory by using the "WriteFLASH()" function. It conforms with the  
   "CodeArray" structure convention of file "Target_Code.s43".  
   Arguments: word *DataArray (Pointer to array with the data)  
   Result:    word (STATUS_OK if verification was successful,  
                    STATUS_ERROR otherwise)  
*/   
word WriteFLASHallSections(word *CodeArray)   
{   
    word s;                     // Number of sections   
    word p;                     // Pointer to data   
   
    for (s = CodeArray[0], p = 1u; s > 0u; s--)   
    {   
        // Write/Verify(PSA) one FLASH section   
        WriteFLASH(CodeArray[p], CodeArray[p+1], &CodeArray[p+2]);   
        if (!VerifyMem(CodeArray[p], CodeArray[p+1], &CodeArray[p+2]))   
        {   
            return(STATUS_ERROR);   
        }   
        // pointer of next section = old pointer + 2 + length of actual section   
        p += CodeArray[p+1] + 2;   
    }   
    return(STATUS_OK);   
}   
   
//----------------------------------------------------------------------------   
/* This function reads one byte/word from a given address in memory  
   Arguments: word Format (F_BYTE or F_WORD)  
              word Addr (address of memory)  
   Result:    word (content of the addressed memory location)  
*/   
word ReadMem(word Format, word Addr)   
{   
    word TDOword;   
   
    HaltCPU();   
   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    if  (Format == F_WORD)   
    {   
        DR_shiftOut16(0x2409);         // Set word read   
    }   
    else   
    {   
        DR_shiftOut16(0x2419);         // Set byte read   
    }   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(Addr);               // Set address   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    SetTCLK();   
   
    ClrTCLK();   
    TDOword = DR_shiftOut16(0x0000);   // shiftOut out 16 bits   
   
    ReleaseCPU();   
    return(Format == F_WORD ? TDOword : TDOword & 0x00FFu);   
}   
   
//----------------------------------------------------------------------------   
/* This function reads an array of words from a memory.  
   Arguments: word StartAddr (Start address of memory to be read)  
              word Length (Number of words to be read)  
              word *DataArray (Pointer to array for the data)  
   Result:    None  
*/   
void ReadMemQuick(word StartAddr, word Length, word *DataArray)   
{   
    byte i;   
   
    // Initialize reading:   
    SetPC(StartAddr-4);   
    HaltCPU();   
   
    ClrTCLK();   
    IR_shiftOut(IR_CNTRL_SIG_16BIT);   
    DR_shiftOut16(0x2409);                    // Set RW to read   
    IR_shiftOut(IR_DATA_QUICK);   
   
    for (i = 0; i < Length; i++)   
    {   
        SetTCLK();   
        ClrTCLK();   
        DataArray[i] = DR_shiftOut16(0x0000); // shiftOut out the data   
                                           // from the target.   
    }   
    ReleaseCPU();   
}   
   
//----------------------------------------------------------------------------   
/* This function performs a mass erase (with and w/o info memory) or a segment  
   erase of a FLASH module specified by the given mode and address.  
   Large memory devices get additional mass erase operations to meet the spec.  
   Arguments: word Mode (could be ERASE_MASS or ERASE_MAIN or ERASE_SGMT)  
              word Addr (any address within the selected segment)  
   Result:    None  
   Remark:    Could be extended with erase check via PSA.  
*/   
void EraseFLASH(word EraseMode, word EraseAddr)   
{   
    word StrobeAmount = 4820;       // default for Segment Erase   
    byte i, loopcount = 1;          // erase cycle repeating for Mass Erase   
    word FCTL3_val = 0xA500;        // ok for all devices; if Info-Seg. A on F2xxx should not be erased   
//  word FCTL3_val = 0xA540;        // only if Info-Seg. A on F2xxx should be erased   
   
    if ((EraseMode == 1u) || (EraseMode == 2u)) // (ERASE_MASS = 1,  ERASE_MAIN = 2) 
    {   
		StrobeAmount = 5300;        // Larger Flash memories require   
		loopcount = 190;             // additional cycles for erase.   
    }   
    HaltCPU();   
   
    for (i = loopcount; i > 0u; i--)   
    {   
        ClrTCLK();   
        IR_shiftOut(IR_CNTRL_SIG_16BIT);   
        DR_shiftOut16(0x2408);         // set RW to write   
        IR_shiftOut(IR_ADDR_16BIT);   
        DR_shiftOut16(0x0128);         // FCTL1 address   
        IR_shiftOut(IR_DATA_TO_ADDR);   
        DR_shiftOut16(0xA502);      // Enable erase mode   
        SetTCLK();   
   
        ClrTCLK();   
        IR_shiftOut(IR_ADDR_16BIT);   
        DR_shiftOut16(0x012A);         // FCTL2 address   
        IR_shiftOut(IR_DATA_TO_ADDR);   
        DR_shiftOut16(0xA540);         // MCLK is source, DIV=1   
        SetTCLK();   
   
        ClrTCLK();   
        IR_shiftOut(IR_ADDR_16BIT);   
        DR_shiftOut16(0x012C);         // FCTL3 address   
        IR_shiftOut(IR_DATA_TO_ADDR);   
        DR_shiftOut16(FCTL3_val);      // Clear FCTL3; F2xxx: Unlock Info-Seg. A by toggling LOCKA-Bit if required,   
        SetTCLK();   
   
        ClrTCLK();   
        IR_shiftOut(IR_ADDR_16BIT);   
        DR_shiftOut16(EraseAddr);      // Set erase address   
        IR_shiftOut(IR_DATA_TO_ADDR);   
        DR_shiftOut16(0x55AA);         // Dummy write to start erase   
        SetTCLK();   
   
        ClrTCLK();   
        IR_shiftOut(IR_CNTRL_SIG_16BIT);   
        DR_shiftOut16(0x2409);         // Set RW to read   
        tcklStrobes(StrobeAmount);  // Provide TCLKs   
        IR_shiftOut(IR_CNTRL_SIG_16BIT);   
        DR_shiftOut16(0x2408);         // Set RW to write   
        IR_shiftOut(IR_ADDR_16BIT);   
        DR_shiftOut16(0x0128);         // FCTL1 address   
        IR_shiftOut(IR_DATA_TO_ADDR);   
        DR_shiftOut16(0xA500);         // Disable erase   
        SetTCLK();   
    }   
    // set LOCK-Bits again   
    ClrTCLK();   
    IR_shiftOut(IR_ADDR_16BIT);   
    DR_shiftOut16(0x012C);         // FCTL3 address   
    IR_shiftOut(IR_DATA_TO_ADDR);   
    DR_shiftOut16(FCTL3_val);      // Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again   
    SetTCLK();   
   
    ReleaseCPU();   
}   
   
//----------------------------------------------------------------------------   
/* This function performs an Erase Check over the given memory range  
   Arguments: word StartAddr (Start address of memory to be checked)  
              word Length (Number of words to be checked)  
   Result:    word (STATUS_OK if erase check was successful, STATUS_ERROR otherwise)  
*/   
word EraseCheck(word StartAddr, word Length)   
{   
    return (VerifyPSA(StartAddr, Length, 0));   
}   
   
//----------------------------------------------------------------------------   
/* This function performs a Verification over the given memory range  
   Arguments: word StartAddr (Start address of memory to be verified)  
              word Length (Number of words to be verified)  
              word *DataArray (Pointer to array with the data)  
   Result:    word (STATUS_OK if verification was successful, STATUS_ERROR otherwise)  
*/   
word VerifyMem(word StartAddr, word Length, word *DataArray)   
{   
    return (VerifyPSA(StartAddr, Length, DataArray));   
}   
   
//------------------------------------------------------------------------   
/* This function blows the security fuse.  
   Arguments: None  
   Result:    word (TRUE if burn was successful, FALSE otherwise)  
*/   
/*
word BlowFuse(void)   
{   
    word mode = VPP_ON_TEST;        // Devices with TEST pin: VPP to TEST   
   
    if(!DeviceHas_TestPin())   
    {   
        // Devices without TEST pin   
        IR_shiftOut(IR_CNTRL_SIG_16BIT);// TDO becomes TDI functionality   
        DR_shiftOut16(0x7201);   
        TDOisInput();   
        mode = VPP_ON_TDI;          // Enable VPP on TDI   
    }   
   
    IR_shiftOut(IR_PREPARE_BLOW);      // Initialize fuse blowing   
    MsDelay(1);   
    VPPon(mode);                    // Switch VPP onto selected pin   
    MsDelay(5);   
    IR_shiftOut(IR_EX_BLOW);           // Execute fuse blowing   
    MsDelay(1);   
   
    // Switch off power to target and wait   
    ReleaseTarget();                // switch VPP and VCC target off   
    MsDelay(200);   
   
    // Check fuse: switch power on, simulate an initial JTAG entry   
    InitTarget();                   // Supply and preset Target Board   
   
    // Return result of "is fuse blown?"   
    return(GetDevice() == STATUS_FUSEBLOWN);   
}   
*/   
//------------------------------------------------------------------------   
/* This function checks if the JTAG access security fuse is blown.  
   Arguments: None  
   Result:    word (STATUS_OK if fuse is blown, STATUS_ERROR otherwise)  
*/   
word IsFuseBlown(void)   
{   
    byte i;   
   
    for (i = 3u; i > 0u; i--)     //  First trial could be wrong   
    {   
        IR_shiftOut(IR_CNTRL_SIG_CAPTURE);   
        if (DR_shiftOut16(0xAAAA) == 0x5555u)   
        {   
            return(STATUS_OK);  // Fuse is blown   
        }   
    }   
    return(STATUS_ERROR);       // fuse is not blown   
}   
/****************************************************************************/   
/*                         END OF SOURCE FILE                               */   
/****************************************************************************/   


