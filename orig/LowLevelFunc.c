    /*==========================================================================*\  
    |                                                                            |  
    | LowLevelFunc.c                                                             |  
    |                                                                            |  
    | Low Level Functions regarding user's Hardware                              |  
    |----------------------------------------------------------------------------|  
    | Project:              MSP430 Replicator                                    |  
    | Developed using:      IAR Embedded Workbench 3.40B [Kickstart]             |  
    |----------------------------------------------------------------------------|  
    | Author:               FRGR                                                 |  
    | Version:              1.5                                                  |  
    | Initial Version:      04-17-02                                             |  
    | Last Change:          09-21-05                                             |  
    |----------------------------------------------------------------------------|  
    | Version history:                                                           |  
    | 1.0 04/02 FRGR        Initial version.                                     |  
    | 1.1 04/02 FRGR        Included SPI mode to speed up shifting function by 2.|  
    | 1.2 06/02 ALB2        Formatting changes, added comments.                  |  
    | 1.3 08/02 ALB2        Initial code release with Lit# SLAA149.              |  
    | 1.4 09/05 SUN1        Software delays redesigned to use TimerA harware;    |  
    |                       see MsDelay() routine. Added TA setup                |  
    | 1.5 12/05 STO         Adapted for 2xx devices with SpyBiWire using 4JTAG   |  
    |----------------------------------------------------------------------------|  
    | Designed 2002 by Texas Instruments Germany                                 |  
    \*==========================================================================*/   
       
    #include "LowLevelFunc.h"   
       
    /****************************************************************************/   
    /* Function declarations which have to be programmed by the user for use    */   
    /* with hosts other than the MSP430F149.                                    */   
    /*                                                                          */   
    /* The following MSP430F149-specific code can be used as a reference as to  */   
    /* how to implement the required JTAG communication on additional hosts.    */   
    /****************************************************************************/   
       
    /*----------------------------------------------------------------------------  
       Initialization of the Controller Board (Master or Host)  
    */   
    void InitController(void)   
    {   
        byte i;   
       
        WDTCTL   = WDTPW + WDTHOLD;     // Stop watchdog timer   
       
        // initialize MCLK = LFXT1 (external crystal)   
        BCSCTL1 |= XTS;                 // ACLK = LFXT1 = HF XTAL   
        do   
        {   
            IFG1 &= ~OFIFG;             // Clear OSCFault flag   
            for (i = 0xFF; i > 0; i--);  // Time for flag to set   
        }   
        while ((IFG1 & OFIFG) != 0);    // OSCFault flag still set?   
        IFG1 &= ~OFIFG;                 // Clear OSCFault flag again   
        BCSCTL2 |= SELM1+SELM0;         // MCLK = LFXT1 (safe)   
       
        // Setup timer_A for hardware delay   
        TACTL &= MC_0;                  // STOP Timer   
        TACTL |= ID_3+TASSEL_1;         // Timer_A source: ACLK/8 = 1MHz   
        TACCR0 = ONEMS;                 // Load CCR0 with delay... (1ms delay)   
       
        MsDelay(50);                    // debounce RST key   
       
        // initialize the Status LEDs control port   
        LEDSEL  &= ~(LEDGREEN | LEDRED);    // no special function, I/O   
        LEDOUT  &= ~(LEDGREEN | LEDRED);    // LEDs are OFF   
        LEDDIR  |=  (LEDGREEN | LEDRED);    // LED pins are outputs   
       
    #ifdef SPI_MODE   
        // initialize the SPI   
        UBR0  = SPI_DIV;                // Load SPI frequency divider   
        UBR1  = 0x00;   
        UMCTL = 0x00;                   // Clear Modulation Register   
        UTCTL = CKPL | SSEL0 | STC | TXEPT; // Load TX control: ACLK (inactive high), no STE   
        UCTL  = SYNC | MM | SWRST;      // Load USART Control: 7bit, SPI, Master   
        UCTL &= ~SWRST;                 // Reset SWRST bit as last action   
        ME   |= USPIE;                  // Enable SPI module   
    #endif   
    }   
       
    /*----------------------------------------------------------------------------  
       This function switches TDO to Input, used for fuse blowing  
    */   
    void TDOisInput(void)   
    {   
        JTAGOUT &= ~TDICTRL1;           // Release TDI pin on target   
        MsDelay(5);                     // Settle MOS relay   
        JTAGOUT |=  TDICTRL2;           // Switch TDI --> TDO   
        MsDelay(5);                     // Settle MOS relay   
    }   
       
    /*----------------------------------------------------------------------------  
       Initialization of the Target Board (switch voltages on, preset JTAG pins)  
        for devices with normal 4wires JTAG  (JTAG4SBW=0)  
        for devices with Spy-Bi-Wire to work in 4wires JTAG (JTAG4SBW=1)  
    */   
    void InitTarget(void)   
    {   
        JTAGSEL  = 0x00;            // Pins all I/Os except during SPI access   
        JTAGOUT  = TEST | TDI | TMS | TCK | TCLK | TDICTRL1 | VCCTGT;   
        JTAGDIR  = TEST | TDI | TMS | TCK | TCLK | TDICTRL1 | TDICTRL2 | VCCTGT;   
       
        VPPSEL  &= ~(VPPONTDI | VPPONTEST | RESET); // No special function, I/Os   
        VPPOUT  &= ~(VPPONTDI | VPPONTEST);         // VPPs are OFF   
        VPPOUT |= RESET;                            // Release target reset   
        VPPDIR  |=  (VPPONTDI | VPPONTEST | RESET); // VPP pins are outputs   
        MsDelay(50);                // Settle MOS relays, target capacitor   
    }   
       
    /*----------------------------------------------------------------------------  
       Release Target Board (switch voltages off, JTAG pins are HI-Z)  
    */   
    void ReleaseTarget(void)   
    {   
        VPPoff();               // VPPs are off (safety)   
        MsDelay(5);             // Settle MOS relays   
        JTAGDIR  =  0x00;       // VCC is off, all I/Os are HI-Z   
        MsDelay(5);             // Settle MOS relays   
    }   
       
    //----------------------------------------------------------------------------   
    /*  Shift a value into TDI (MSB first) and simultaneously shift out a value  
        from TDO (MSB first).  
        Note:      When defining SPI_MODE the embedded SPI is used to speed up by 2.  
        Arguments: word Format (number of bits shifted, 8 (F_BYTE) or 16 (F_WORD))  
                   word Data (data to be shifted into TDI)  
        Result:    word (scanned TDO value)  
    */   
    word Shift(word Format, word Data)   
    {   
        word tclk = StoreTCLK();        // Store TCLK state;   
        word TDOword = 0x0000;          // Initialize shifted-in word   
        word MSB = 0x0000;   
       
    #ifdef SPI_MODE                     // Shift via SPI pins, entry: TCK=1, TMS=0   
        JTAGSEL |= (TDI | TDO | TCK);   // Function select to SPI module   
        // Process 8 MSBs if 16 bits   
        if (Format == F_WORD)   
        {   
            UCTL |= CHAR;               // SPI: set 8bit mode   
            UTXBUF = (byte)(Data >> 8);   // Write TX Buffer: 8 MSBs   
            while ((UTCTL & TXEPT) == 0);   
            MSB = (URXBUF < 8);      // Get RX Buffer: 8 MSBs   
        }   
        // Process higher 7 LSBs   
        UCTL &= ~CHAR;                  // SPI: set 7bit mode   
        UTXBUF = (byte)(Data);          // Write TX Buffer: 7(+1) LSBs   
        while ((UTCTL & TXEPT) == 0);   
        TDOword = MSB + (URXBUF < 1);    // Combine 15 MSBs   
        JTAGSEL &= ~(TDI | TDO | TCK);  // Function select back to ports   
        // process LSB discretely due to TMS   
        ((Data & 0x01) == 0) ? ClrTDI() : SetTDI();   
        SetTMS();                       // Last bit requires TMS=1   
        ClrTCK();   
        SetTCK();   
        if (ScanTDO() != 0)   
            TDOword++;   
       
    #else                               // Shift via port pins, no coding necessary   
        volatile word i;   
        (Format == F_WORD) ? (MSB = 0x8000) : (MSB = 0x80);   
        for (i = Format; i > 0; i--)   
        {   
            ((Data & MSB) == 0) ? ClrTDI() : SetTDI();   
            Data <= 1;   
            if (i == 1)                 // Last bit requires TMS=1   
               SetTMS();   
            ClrTCK();   
            SetTCK();   
            TDOword <= 1;                // TDO could be any port pin   
            if (ScanTDO() != 0)   
                TDOword++;   
        }   
    #endif   
       
        // common exit   
        RestoreTCLK(tclk);              // restore TCLK state   
       
        // JTAG FSM = Exit-DR   
        ClrTCK();   
        SetTCK();   
        // JTAG FSM = Update-DR   
        ClrTMS();   
        ClrTCK();   
        SetTCK();   
        // JTAG FSM = Run-Test/Idle   
        return(TDOword);   
    }   
       
    /*---------------------------------------------------------------------------  
       Delay function (resolution is 1 ms)  
       Arguments: word millisec (number of ms, max number is 0xFFFF)  
    */   
    void MsDelay(word milliseconds)   
    {   
       word i;   
       for(i = milliseconds; i > 0; i--)   
       {   
            TACCTL0 &= ~CCIFG;          // Clear the interrupt flag   
            TACTL |= TACLR+MC_1;            // Clear & start timer   
            while ((TACCTL0 & CCIFG)==0);   // Wait until the Timer elapses   
            TACTL &= ~MC_1;             // Stop Timer   
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
            _NOP();   
            _NOP();   
        }   
        while (--microeconds > 0);   
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
    void TCLKstrobes(word Amount)   
    {   
        volatile word i;   
       
        for (i = Amount; i > 0; i--)     // This implementation has 14 body cycles!   
        {   
            JTAGOUT |=  TCLK;       // Set TCLK   
            _NOP();                 // Include NOPs if necessary (min. 3)   
            _NOP();   
            _NOP();   
            JTAGOUT &= ~TCLK;       // Reset TCLK   
        }   
    }   
       
    /*----------------------------------------------------------------------------  
       This function controls the status LEDs depending on the status  
       argument. It stops program in error case.  
       Arguments: word status (4 stati possible for 2 LEDs)  
                  word index (additional number for detailed diagnostics or  
                              watch variable during debugging phase)  
    */   
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
       
    /*----------------------------------------------------------------------------  
       This function performs a Trigger Pulse for test/development  
    */   
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
       
    /****************************************************************************/   
    /*                         END OF SOURCE FILE                               */   
    /****************************************************************************/   

