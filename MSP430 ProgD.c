/*==========================================================================*\  
|                                                                            |  
| MSP430 ProgD.c                                                             |  
|                                                                            |  
| Main programfile                                                           |  
|----------------------------------------------------------------------------|  
| Project:              MSP430 ProgD                                         | 
| Developed using:      Microchip MPLAB IDE v8.66 + HITECHC Toolchain 9.83   |  
|----------------------------------------------------------------------------|  
| Author:               STO                                                  |  
| Version:              1.6                                                  |  
| Initial Version:      04-17-02                                             |  
| Last Change:          13-13-05                                             |  
|----------------------------------------------------------------------------|  
| Version history:                                                           |  
| 1.0 04/12 STR        Initial version.                                      |   
|----------------------------------------------------------------------------|  
| Designed 2012 by Steven Rott (STR)                                         |  
\*==========================================================================*/   

#include "p18f4620.h"
#include "JTAGfunc.h"   
#include "MSP430 ProgD.h" 
#include "IO.h"
#include "LowLevelfunc.h"

// Configuration bits for PIC16f887
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config PWRT = ON
#pragma config BOREN = OFF
#pragma config PBADEN = OFF
#pragma config STVREN = OFF
#pragma config XINST = OFF

#pragma code low_vector=0x18
void lowInterrupt(void)
{
    _asm GOTO rxInISR _endasm
}

// This is needed to convert the hexadecimal content of the memory to
// rom character set of the display so the user can see whats in the
// memory. This is stored in RAM...

#pragma rom codepad
    rom word codepad[2][16]=
    {   '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
        0x0000,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000,0x8000,
        0x9000,0xA000,0xB000,0xC000,0xD000,0xE000,0xF000
    };
    #pragma rom

/*
#pragma rom byteCodepad
    rom byte byteCodepad[2][16]=
    {   '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
        0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xA0,0xB0,0xC0,0xD0,0xE0,0xF0
    };
    #pragma rom
*/
/*   the type of firmware to work with...

     the 3dimensional array 'dataLocator' contains the adresses of the needed information
     depending on the firmware version (MAR2009, JAN2012) and device type (INT_LN_R, RN_ER).

                     ___________ ________ ____________ _____________ __________                                     
                     .......    |  NETID |   SGMT-IN  |   SGMT-OUT  |   FREQ   |
                     -----------|--------|------------|-------------|--------- |                                              
                  ___________ ________ ____________ _____________ __________   |                                  
                  RN_ER(0x02)|  NETID |   SGMT-IN  |   SGMT-OUT  |   FREQ   |--|  
                  -----------|--------|------------|-------------|----------|  |                                            
            ______________ _________ ____________ _____________ _________   |--|   
            INT_LN_R(0x01)|  NETID  |   SGMT-IN  |    SGMT-OUT |   FREQ  |--|  |   
            --------------|---------|------------|-------------|---------|  |--|                      
     (0x01) MAR2009       |  0x8010 |   0x8012   |    N/A      |  N/A    |--|           
            --------------|---------|------------|-------------|---------|  |                                     
     (0x02) JAN2012       |  N/A    |   N/A      |    N/A      |  N/A    |--|   
            --------------|---------|------------|-------------|---------|                                     
     (0x03)               |         |            |             |         |      
            --------------|---------|------------|-------------|---------|                                       
            ...
*/

const word data13[4]={0x0A00,0xFF00,0x0000,0x0000};
const word data1[8]={0xA06A,0x0100,0x0008,0xF008,0xA0DA,0x0004,0x0012,0xA27E};

typedef struct{
        word net_id_adress;
        word sgmt_in_adress;
        word sgmt_out_adress;
        word freq_adress;
}device;

device int_ln_r;

#pragma rom dataLocator
    static rom word dataLocator[4][5]=			// [devicetype], [datatype]
    {   
		0,0,0,0,0,
        INT_LN_R_NETID, INT_LN_R_SGMT_IN, 0, INT_LN_R_FREQ_A, INT_LN_R_FREQ_B,     
        RN_ER_2009_NETID, RN_ER_2009_SGMT_IN, RN_ER_2009_SGMT_OUT, RN_ER_2009_FREQ_A,RN_ER_2009_FREQ_B,   
        0,0,0,0,0     
    };
    #pragma rom

// ISR for recieve

byte charcount=0;
byte length=0;
byte serialdata[8];
word adrSRAM=0x0000;

struct StatusBits {
  unsigned rx_begin:1;
  unsigned high_nibble:1;
  unsigned ready:1;
  unsigned rx_len:1;
  unsigned high_byte:1;
  unsigned rx_adress:1;
  unsigned rx_data:1;
  unsigned rx_ack:1;
} STATbits={0,1,0,0,1,0,0,0};

#pragma interruptlow rxInISR
void rxInISR(void)
{
    byte rxbuf=RCREG;

	if(!STATbits.ready)
	{
		txOut(13,0); 
		txOut(0,(rom byte*)"Device not ready yet.");
		txOut('\n',0);
		txOut(13,0); 
		txOut(0,(rom byte*)"Please start 'programming mode' first,");
		txOut('\n',0);
		txOut(13,0);
		txOut(0,(rom byte*)"by pressing '014'");
 		txOut('\n',0);
		return;
	}

/*-----------------------------------------------------------------
Recieving the bytecount from the USART...
------------------------------------------------------------------  */

	if(STATbits.rx_begin)
	{
		serialdata[charcount]=rxbuf;

		charcount++;

		if(charcount==8u)
		{
			STATbits.rx_data=TRUE;
			STATbits.rx_begin=FALSE;
			charcount=0;

			length|=ascii2hex(serialdata[0]);
			length<<=4;
			length|=ascii2hex(serialdata[1]);
	
			length+=length;

			adrSRAM|=ascii2hex(serialdata[2]);
			adrSRAM<<=4;
			adrSRAM|=ascii2hex(serialdata[3]);
			adrSRAM<<=4;
			adrSRAM|=ascii2hex(serialdata[4]);
			adrSRAM<<=4;
			adrSRAM|=ascii2hex(serialdata[5]);
		}
	}

	if(STATbits.rx_data&&length)
	{
		sramWrite(adrSRAM,rxbuf);
		length--;
		adrSRAM++;
	}

    if(rxbuf==':')
	{
        STATbits.rx_begin=TRUE;
		STATbits.rx_ack=TRUE;
		STATbits.high_nibble=TRUE;
		STATbits.rx_len=TRUE;
		length=0;
		charcount=0;
	}
}

/*---------------------------------------------------------------------------  
   This is the main routine
  
   Arguments:   
   Result:
*/

void main(void)
{
	byte i=0x03;

	byte serialData;

	byte* dataP;

	word iterA;	
	byte iterB=0;
	word adress;

	byte temp_2=0;
	byte firmwareVersion=0x00;
	word cmd;

    byte keyStroke;

	initController();

	// PORT A,B,C*,D* and E as output

    // PORTA.0 = !WR        (Display and SRAM)
    // PORTA.1 = !RD  		(Display and SRAM)
    // PORTA.2 = CS1  		(CS LCD)
    // PORTA.3 = CD 		(Command / data input of the display controller)
    // PORTA.4 = LAL   		(AdressLatch of A8..15)
    // PORTA.5 = HAL 		(AdressLatch of A0..7)
    // PORTA.6 = AEN 		(16bit Adress output enable)
    // PORTA.7 = CS2		(CS SRAM)
			
	TRISA = 0b00000000;     // used for the display and sram (bus control port)
	LATA  = 0b11001111;

	TRISB = 0b11111111;     // used as multiplexed adress/databus (for the display and the sram)

	TRISC = 0b11000001;     // *RC0 - RC5 JTAG Interface PortC.0 is TDO, RC6 UART TX, RC7 UART RX
	LATC  = 0b00111110;

    TRISD = 0b11110000;     // *used for the matrix keypad
	TRISE = 0b00000000;     // unused for now...

    // initialize the display controller, (re)start the display and
    // draw the "mainmenu-frame"		
	displayInit();

    // show me the Device is working...

	txOut(0,(rom byte*)"MSP430 ProgD v0.01");
	txOut('\n',0); txOut(13,0);

	while(1)
	{
    	switch(getWholeCommand())           // get the user command from the keyboard...
		{                                   // and switch to the corresponding (sub)menu.
			case 0x0010:
			{               //             "<   max-length    >"
				printToScreen(0,0,(rom byte*)">010: applications ",1,1);
				printToScreen(0,0,(rom byte*)">020: settings     ",1,2);
				printToScreen(0,0,(rom byte*)"                   ",1,3);
				printToScreen(0,0,(rom byte*)">030: help         ",1,4);
				printToScreen(0,0,(rom byte*)"          ",1,6);
			}break;

			case 0x0100:
			{                 //           "<   max-length    >"
				printToScreen(0,0,(rom byte*)">011: clone device ",1,1);
				printToScreen(0,0,(rom byte*)">012: read config  ",1,2);
				printToScreen(0,0,(rom byte*)">013: routersearch ",1,3);
				printToScreen(0,0,(rom byte*)">014: progr. mode  ",1,4);
				printToScreen(0,0,(rom byte*)"                   ",1,5);
				printToScreen(0,0,(rom byte*)"<001: back       ",1,6);
	
			}break;

			case 0x0110:
			{                 //           "<   max-length    >"
				printToScreen(0,0,(rom byte*)"reading FW data... ",1,1);
				printToScreen(0,0,(rom byte*)"                   ",1,2);
				printToScreen(0,0,(rom byte*)"                   ",1,3);
				printToScreen(0,0,(rom byte*)"                   ",1,4);
				printToScreen(0,0,(rom byte*)"<010: back",1,6);
				

				if(GetDevice()!=1u)
				{                //            "<   max-length    >"
					printToScreen(0,0,(rom byte*)" ERROR 0x1F!      ",1,1);
					printToScreen(0,0,(rom byte*)" INVALID JTAG-ID! ",1,2);
					printToScreen(0,0,(rom byte*)" no connection?   ",1,3);
					printToScreen(0,0,(rom byte*)"<012: retry   ",1,5);
					break;
				}

				else
				{
						msDelay(1000);
						//EraseFLASH(3,0x8000);

					    txOut('\n',0); 	txOut(13,0);
						txOut(0,(rom byte*)"here comes the data...");

						//	iterA=adrSRAM;
						adrSRAM=0x8000;
	                    txOut('\n',0); 	txOut(13,0);
						for(adrSRAM=0x8000;adrSRAM<=0x8100;adrSRAM++)
						{
							serialData=sramRead(adrSRAM);
							txOut(serialData,0);
						}

		        ReleaseDevice(V_RESET);
				}	
	
				//WriteFLASH(0x8000,8,data1);
				//WriteFLASH(0x8080,4,data13);

			}break;

			case 0x0120:
			{	
			                 //            "<   max-length    >"
				printToScreen(0,0,(rom byte*)" analysing FW ver ",1,1);
				printToScreen(0,0,(rom byte*)" please wait...   ",1,2);
				printToScreen(0,0,(rom byte*)"                  ",1,3);
				printToScreen(0,0,(rom byte*)"                  ",1,4);
				printToScreen(0,0,(rom byte*)"                  ",1,5);
				printToScreen(0,0,(rom byte*)"<010: back",1,6);
				
				if(GetDevice()!=1u)
				{                //            "<   max-length    >"
					printToScreen(0,0,(rom byte*)" ERROR 0x1F!      ",1,1);
					printToScreen(0,0,(rom byte*)" INVALID JTAG-ID! ",1,2);
					printToScreen(0,0,(rom byte*)" no connection?   ",1,3);
					printToScreen(0,0,(rom byte*)"<012: retry   ",1,5);
					break;
				}

				else
				{

					firmwareVersion=getFirmwareVersion();
                    cacheFirmware(firmwareVersion);
                    drawFirmwareInfo(firmwareVersion);

					msDelay(50);

		            ReleaseDevice(V_RESET);

				}
			}break;

			case 0x0130:
			{                //            "<   max-length    >"
				printToScreen(0,0,(rom byte*)" reading current  ",1,1);
				printToScreen(0,0,(rom byte*)" configuration... ",1,2);
				printToScreen(0,0,(rom byte*)"                  ",1,3);
				printToScreen(0,0,(rom byte*)"                  ",1,4);
				printToScreen(0,0,(rom byte*)"<010: back",1,6);


				msDelay(1000);

                cacheFirmware(INT_LN_R);
               	drawFirmwareInfo(INT_LN_R);
				

      				        //             "<   max-length    >"
   							//             "<             max-length            >"
			//	printToScreen(0,0,(rom byte*)" edit new config                     ",1,1);
			//	printToScreen(0,0,(rom byte*)"                                     ",1,2);
			//	printToScreen(0,0,(rom byte*)"                                     ",1,3);
				printToScreen(0,0,(rom byte*)" please specify new values by        ",1,8);
				printToScreen(0,0,(rom byte*)" pressing the 'E'key. Press it again ",1,9);
				printToScreen(0,0,(rom byte*)" when finished.                      ",1,10);

				while(waitForSingleKey(ASCII)!='E');
					usDelay(1000);

				keyStroke=0xFF;
				while(waitForSingleKey(ASCII)!=0xFF)
					usDelay(1000);

				for(iterA=0u; iterA<=3u; iterA++){
					display(2,15+iterA,1,0x21);		    // Cursor Pointer (x,y)
					while(keyStroke==0xFF)
						keyStroke=waitForSingleKey(ASCII);
					printToScreen(keyStroke,0,0,15+iterA,1); 
					keyStroke=0xFF;
					while(waitForSingleKey(ASCII)!=0xFF)
						usDelay(100);
				}

				for(iterB=0u; iterB<=63u; iterB++){
					temp_2=sramRead(iterB);
					temp_2>>=4;
					txOut(hex2ascii(temp_2&0x0F),0);
					txOut(hex2ascii((sramRead(iterB)&0x0F)),0);
				}

				EraseFLASH(3,0x8000);

				writeFirmware(0x8000,3);


			}break;

			case 0x0140:
			{                //            "<   max-length    >"
   							//             "<             max-length            >"
				printToScreen(0,0,(rom byte*)" You need to erase ",1,1);
				printToScreen(0,0,(rom byte*)" the target before ",1,2);
				printToScreen(0,0,(rom byte*)" you can continue. ",1,3);
				printToScreen(0,0,(rom byte*)" (E)rase. (A)bort  ",1,4);
				printToScreen(0,0,(rom byte*)"<010: back",1,6);

				txOut('\n',0);
				txOut(13,0);
				txOut(0,(rom byte*)"Device ready for ERASE...");

                while(keyStroke!=3u||4u)
                {

                    keyStroke=waitForSingleKey(ASCII);

                    if(keyStroke==4u)
                    {  
						txOut('\n',0);
						txOut(13,0);
						txOut(0,(rom byte*)"ERASING...");
/*
						if(GetDevice()!=1u)
						{                //            "<   max-length    >"
							printToScreen(0,0,(rom byte*)" ERROR 0x1F!      ",1,1);
							printToScreen(0,0,(rom byte*)" INVALID JTAG-ID! ",1,2);
							printToScreen(0,0,(rom byte*)" no connection?   ",1,3);
        					printToScreen(0,0,(rom byte*)"                  ",1,4);
							printToScreen(0,0,(rom byte*)"<014: retry   ",1,5);
							break;
						}
*/
                        printToScreen(0,0,(rom byte*)" ERASING...        ",1,1);
        				printToScreen(0,0,(rom byte*)"                   ",1,2);
        				printToScreen(0,0,(rom byte*)"                   ",1,3);
        				printToScreen(0,0,(rom byte*)"                   ",1,4);

						for(adress=0x8000;adress<=0xFE00;adress+=0x100)
						{
                     //  		EraseFLASH(3,adress);
						}

                        printToScreen(0,0,(rom byte*)" DONE...           ",1,1);
	                    
                        txOut('\n',0); 	txOut(13,0);
                        txOut(0,(rom byte*)"ready...");

						STATbits.ready=1;
						STATbits.rx_ack=FALSE;
						adrSRAM=0x0000;
						
						while(!STATbits.rx_ack)
						{
	                        printToScreen(0,0,(rom byte*)" Waiting for FW    ",1,1);
	        				printToScreen(0,0,(rom byte*)" data. Press 'F'   ",1,2);
	        				printToScreen(0,0,(rom byte*)" Button when the   ",1,3);
	        				printToScreen(0,0,(rom byte*)" transfer is done. ",1,4);
						}

                        printToScreen(0,0,(rom byte*)" receiving DATA... ",1,1);
        				printToScreen(0,0,(rom byte*)"                   ",1,2);
        				printToScreen(0,0,(rom byte*)"                   ",1,3);
        				printToScreen(0,0,(rom byte*)"                   ",1,4);

                    }

                    else if(keyStroke==3u)
					{
                        cmd=0x0100;
						txOut('\n',0);
						txOut(13,0);
						txOut(0,(rom byte*)"Transmission terminated...");

						printToScreen(0,0,(rom byte*)">010: applications ",1,1);
						printToScreen(0,0,(rom byte*)">020: settings     ",1,2);
						printToScreen(0,0,(rom byte*)"                   ",1,3);
						printToScreen(0,0,(rom byte*)">030: help         ",1,4);
						printToScreen(0,0,(rom byte*)"          ",1,6);

	                    txOut('\n',0); 	txOut(13,0);
						txOut(0,(rom byte*)"here comes the data...");

					//	iterA=adrSRAM;
						adrSRAM=0x8000;
	                    txOut('\n',0); 	txOut(13,0);
						for(adrSRAM=0x8000;adrSRAM<=0x8100;adrSRAM++)
						{
							serialData=sramRead(adrSRAM);
							txOut(serialData,0);
						}break;

					}
                }
			}break;
		}
	}
}

/*---------------------------------------------------------------------------  
   This routine is used to write data from sram to flash
  
   Arguments:   
   Result:
*/

byte writeFirmware(word startAdr, word lenght)
{
	byte flashDataByte[16];
	word* wDataP;
	word flashDataWord[8]={0,0,0,0,0,0,0,0};
	byte serialData;
	byte serialDataBackup;
	byte iterA, iterB;

	adrSRAM=0x0000;

	txOut('\n',0); 	txOut(13,0);

	for(iterB=0u; iterB<=length;iterB++){	// The data is written to the flash in lines...
		for(iterA=0u;iterA<=15u;iterA++){	// of 8 Words each.
			serialData=sramRead(adrSRAM);
			flashDataByte[iterA]=serialData;
			serialDataBackup=serialData;
			serialData>>=4;
	
			if(!(iterA%2)&&iterA)			// Print out a space char each word (rs232 debugging)
				txOut(' ',0);
	
			txOut(hex2ascii(serialData&=0x0F),0);	// Print out each Data written to flash...
			serialData=serialDataBackup;
			txOut(hex2ascii(serialData&=0x0F),0);
			
			if(!(iterA%2)){					// convert sram bytes to little endian format word
											// i.e. A6 6E -> 6EA6
				flashDataWord[iterA/2]|=serialDataBackup;	
				flashDataWord[iterA/2]<<=8;			
			}

			else{
				flashDataWord[iterA/2]|=serialDataBackup;				
			}

			adrSRAM++;
		}
		wDataP=flashDataWord;
		WriteFLASH(startAdr+(iterB*0x0010),8u,wDataP);
	}
	ReleaseDevice(V_RESET);
}


/*---------------------------------------------------------------------------  
   In the first step, here i'll try to find out which FW version i have.
   This is important as the information needed is located in an different
   location in the flash memory. The Location differs in every FW ver.

   Additionally i need to figure out whats the type of component connected
   as the Locknodes dont need two segments like the routers...
  
   Arguments: none
   Result: 0x01 means: I found a WN.RN.ER with FW of 2009
           0x02 means: I found a WN.RN.ER with FW of 2012

           0x00 means: I cannot find any known FW
   TODO:   add the different nodetypes and FW versions
*/


byte getFirmwareVersion(void)
{
	word data[6];

	word i;

	for(i=0xA200;i<0xA4FF;i+=2u)    //nothing found? maybe another FW ver!
	{
		
	
		data[0]=ReadMem(F_WORD,i);

		if(data[0]==0x7541u)
			return RN_ER_2009;

	}	

	for(i=0xEA00;i<0xEDFF;i+=2u)    //nothing found? maybe another FW ver!
	{
		
	
		data[0]=ReadMem(F_WORD,i);

		if(data[0]==0x614Au)
			return RN_ER_2012;

	}

return 0;                           //i did not found any known FW ver info segment! 
}

/*---------------------------------------------------------------------------  
    In the second step, i'll put the corresponding data segment to the
    SRAM. The data will get lost otherwise because of the need to
    erase the specific flash segment before write. The PIC18F4620 has
    4K of RAM. I use a hardware attached 64K 15ns SRAM (61512 AK)

    Arguments: 	The "Firmware Version". Which means basically the type of Device.
				Depending on this, different adress locations needed to
				be cached.

    Result: 0x01 means: a WN.RN.ER with FW of 2009
            0x02 means: a WN.RN.ER with FW of 2012
            0x30 means: the internal LN.R

            0x00 means: I cannot find any known FW
    TODO:   add the different nodetypes and FW versions
*/

byte cacheFirmware(byte fwVer)
{
	word flashDataWord=0x0000;
	word flashDataWordBackup=0x0000;
	byte flashDataByte=0x00;
	byte flashDataByteBackup=0x00;

	word adrSRAM=0x8000;
    word* ramP;
    word iterA=0x0000;
	word iterB=0x0000;

    switch (fwVer)
    {
        case RN_ER_2009:
        {
        	//ramP=firmwareData1;

        	for(adrSRAM=0x8000;adrSRAM<0x808F;adrSRAM+=2u)
        	{
        		*ramP=ReadMem(F_WORD,adrSRAM);
                ramP++;
        	}
            return STATUS_OK;
        }break;

        case RN_ER_2012:
        {
        	//ramP=firmwareData1;

        	for(adrSRAM=0x8000;adrSRAM<0x806F;adrSRAM+=2u)
        	{
        		*ramP=ReadMem(F_WORD,adrSRAM);
                ramP++;
        	}
            return STATUS_OK;
        }break;

        case INT_LN_R:
        {
        	//ramP=firmwareData1;
        	for(adrSRAM=0x8000u;adrSRAM<=0x803Fu;adrSRAM+=2u){
				flashDataWord=ReadMem(F_WORD,adrSRAM);
				flashDataWordBackup=flashDataWord;

				for(iterB=0u; iterB<=1u; iterB++){
					flashDataByteBackup|=flashDataWordBackup;
					flashDataByteBackup>>=4;
					txOut(hex2ascii(flashDataByteBackup&0x0F),0);
					txOut(hex2ascii(flashDataWordBackup&0x000F),0);
					flashDataWordBackup>>=8;
				}

				txOut(' ',0);

				flashDataByte=flashDataWord;
				flashDataWord>>=8;
        		sramWrite(iterA,flashDataByte);

				flashDataByte=flashDataWord;
        		sramWrite(iterA+1,flashDataByte);
				iterA+=2;

        	}
			
            return STATUS_OK;
        }break;

        default:
            return STATUS_ERROR;
    }
}

/*---------------------------------------------------------------------------  
    Here, the internals of the components firmware is brought to the user
    via the screen.

    Arguments:      

    Result: STATUS_ERROR (0)
            STATUS_OK (1)

    TODO:   add the different nodetypes and FW versions
*/

byte drawFirmwareInfo(byte firmwareVersion)
{
    switch (firmwareVersion)
	{
		case NOT_KNOWN:
		{
			printToScreen(0,0,(rom byte*)" ERROR 0x0F!      ",1,1);
			printToScreen(0,0,(rom byte*)" UNKOWN FW-ver!   ",1,2);
			printToScreen(0,0,(rom byte*)"<012: retry   ",1,5);

			return STATUS_ERROR;
			
		}break;

		case RN_ER_2009:
		{
		                 //            "<   max-length    >"
			printToScreen(0,0,(rom byte*)" FOUND WN.RN.XX   ",1,1);
			printToScreen(0,0,(rom byte*)" FWver 03.08.2009 ",1,2);
			printToScreen(0,0,(rom byte*)"                ",1,5);

			msDelay(5000);
		}break;

		case RN_ER_2012:
		{
		                 //            "<   max-length    >"
			printToScreen(0,0,(rom byte*)" FOUND WN.RN.XX   ",1,1);
			printToScreen(0,0,(rom byte*)" FWver 03.01.2012 ",1,2);
			printToScreen(0,0,(rom byte*)"                ",1,5);
			
			msDelay(5000);

		}break;

		case INT_LN_R:
		{
/*		                 //            "<   max-length    >"
			printToScreen(0,0,(rom byte*)" Please set the   ",1,1);
			printToScreen(0,0,(rom byte*)" NETID, SGMT and  ",1,2);
			printToScreen(0,0,(rom byte*)" Freq. and press  ",1,3);
			printToScreen(0,0,(rom byte*)" the 'E' key.     ",1,4);
			
			msDelay(3000);
*/
			cls();

			drawInputSgmt(INT_LN_R);
			drawNetID(INT_LN_R);
			drawFreq(INT_LN_R);

		}break;
	}
	return STATUS_OK;
}


/*---------------------------------------------------------------------------  
   This is the xy routine
  
   Arguments:   
   Result:
*/

void drawFreq(byte firmwareVersion){

	// the carrier freq is coded as 3 bytes in 2 words, thus the last byte in word 2
	// is not used to determine the carrier frequency. each word is little-endian and
	// must be converted to big-endian before calculating the correct carrier frequency
	// i.e. word 1 = 6E21; word 2 = 99D5 the correct register value is 216ED5 = 2191061 dec :)
	// the carrier freq for ti's c1101 is then calc with 
	// the equation 26.000.000 * 2191061 / 65536 = 869(,)256.387 (M)Hz
	
	byte byteContainer[5]={0,0,0,0,0};
    byte cntByteIter;
    byte indexCodepad;				// indexcounter for the codepad conversion hex <> ascii
	byte cntHLByteIter;
	byte cntIterForDecData=0;

	byte* data_ptr;

	word rawData[3];
    byte decodedData[10];

	dword carrierFreqReg=0;
	ufloat carrierFreq=0;

	rawData[0]=ReadMem(F_WORD,dataLocator[firmwareVersion][FREQ_A]);
	rawData[1]=ReadMem(F_WORD,dataLocator[firmwareVersion][FREQ_B]);

	byteContainer[0]|=rawData[0];		// the order of loading the byteContainer defines which byte
	rawData[0]>>=8;						// of each word is displayed and stored first
	byteContainer[1]|=rawData[0];		// important for calculating !

	byteContainer[2]|=rawData[1];
	//rawData[1]>>=8;					// no need to touch the last byte (yet)
	//byteContainer[3]|=rawData[1];

	for(cntByteIter=0; cntByteIter<3u; cntByteIter++){
		carrierFreqReg<<=8;
		carrierFreqReg|=byteContainer[cntByteIter];
	}

	switch (carrierFreqReg){

	case 0x00216ed5u:
		printToScreen(0,0,(rom byte*)"CarrierFreq: 869,2563MhZ         ",2,4);
	break;

	case 0x002165c2u:
		printToScreen(0,0,(rom byte*)"CarrierFreq: 868,3347MhZ         ",2,4);
	break;

	default:
	printToScreen(0,0,(rom byte*)"CarrierFreq: NOT FOUND         ",2,4);
	}
}

/*---------------------------------------------------------------------------  
   This is the xy routine
  
   Arguments:   
   Result:
*/

void drawInputSgmt(byte firmwareVersion){

    byte j;
    byte k;

	byte* data_ptr;

	word data[2];
    byte decodedData[9];

	data[0]=ReadMem(F_WORD,dataLocator[firmwareVersion][SEGM_IN]);//
	data[1]=data[0];

	for(j=0; j<4u; j++)
	{

		data[0]=data[1];

		data[0]&=0xF000;

		for(k=0u; k<=15u; k++)
		{
			if(codepad[1][k]==data[0])
				decodedData[j]=codepad[0][k];
		}
		
		data[1]<<=4;
	}

	decodedData[4]='\0';
			   	//             "<             max-length            >"
	printToScreen(0,0,(rom byte*)"InputSGMT:       ",2,1);
	data_ptr=decodedData;
	printToScreen(0,data_ptr,0,15,1);
}

/*---------------------------------------------------------------------------  
   This is the xy routine
  
   Arguments:   
   Result:
*/

void drawOutputSgmt(byte firmwareVersion){

    byte j;
    byte k;

	byte* data_ptr;

	word data[2];
    byte decodedData[9];

    data[0]=ReadMem(F_WORD,dataLocator[firmwareVersion][SEGM_OUT]); //0x8024
	data[1]=data[0];

	for(j=0; j<4u; j++)
	{

		data[0]=data[1];

		data[0]&=0xF000;

		for(k=0; k<=15u; k++)
		{
			if(codepad[1][k]==data[0])
				decodedData[j]=codepad[0][k];
		}
		
		data[1]<<=4;
	}

	decodedData[4]='\0';
   	//             "<             max-length            >"
	printToScreen(0,0,(rom byte*)"OutputSGMT:       ",2,2);
	data_ptr=decodedData;
	printToScreen(0,data_ptr,0,15,2);
}

/*---------------------------------------------------------------------------  
   This is the xy routine
  
   Arguments:   
   Result:
*/

void drawNetID(byte firmwareVersion){

    byte j;
    byte k;

	byte* data_ptr;

	word data[2];
    byte decodedData[9];

	data[0]=ReadMem(F_WORD,dataLocator[firmwareVersion][NETID]);//
	data[1]=data[0];

	for(j=0; j<4u; j++)
	{

		data[0]=data[1];

		data[0]&=0xF000;

		for(k=0u; k<=15u; k++)
		{
			if(codepad[1][k]==data[0])
				decodedData[j]=codepad[0][k];
		}
		
		data[1]<<=4;
	}

	decodedData[4]='\0';
			   	//             "<             max-length            >"
	printToScreen(0,0,(rom byte*)"NetID:       ",2,3);
	data_ptr=decodedData;
	printToScreen(0,data_ptr,0,15,3);
}

/*---------------------------------------------------------------------------  
   This is the xy routine
  
   Arguments:   
   Result:
*/

byte ascii2hex(byte ascii)
{
	//byte temp=ascii;

	if(ascii>=48u||ascii<=57u)
		return ascii-48;

	else if(ascii>=65u||ascii<=70u)
		return ascii-55u;

	else
		return 0;
}

/*---------------------------------------------------------------------------  
   This is the xy routine
  
   Arguments:   
   Result:
*/

byte hex2ascii(byte hex)
{
	//byte temp=ascii;

	if(hex>=0u&&hex<=9u)
		return hex+=48;

	else if(hex>=10u&&hex<=15u)
		return hex+=55;

	else
		return 0;
}

//#######################################################################################
//# REST IN PEACE 																	 	#
//# HERE REST ALL UNUSED FUNCTIONS AND 'FUNCTION PROTOTYPES' THAT ARE NO LONGER USED    #
//# OR WHICH WERE REPLACED BY BETTER SOLUTIONS                                          #
//#######################################################################################


//	carrierFreq = carrierFreqReg;
//	carrierFreq = carrierFreq / 65536 * 26000;
//	carrierFreqReg = carrierFreq;
/*
	for(cntByteIter=0; cntByteIter<3u; cntByteIter++)	// convert memory content to displayable ascii data
	{													// by using a convert table hex <--> ascii
		byteContainer[4]=byteContainer[cntByteIter];	// load byte to convert in the "working position" in array
														// "byteContainer". Each byte must be broken up in nibbles
		rawData[2]=byteContainer[4];					// same as above for the words. each word must be reviewed twice
		rawData[2]<<=8;									// for compatibility reasons convert back from byte to word
		rawData[2]&=0xF000;								// just convert upper nibble

		for(cntHLByteIter=0u; cntHLByteIter<2u; cntHLByteIter++){
			for(indexCodepad=0u; indexCodepad<=15u; indexCodepad++){
				if(codepad[1][indexCodepad]==rawData[2]){
					decodedData[cntIterForDecData]=codepad[0][indexCodepad];
					rawData[2]=byteContainer[4];		// reload the word "working register"
					rawData[2]<<=12;					// and select the lower nibble of the current byte
					cntIterForDecData++;				// next ascii char
					break;								// no need to continue searching in the codepad
				}
			}
		}
	}


	for(cntByteIter=0u; cntByteIter<6u;cntByteIter++){
		byteContainer[3]=carrierFreqReg;
		byteContainer[3]&=0x0F;
		decodedData[cntByteIter]=hex2ascii(byteContainer[3]);
		carrierFreqReg>>=4;
	}

	decodedData[6]='\0';
			   	//             "<             max-length            >"
	printToScreen(0,0,(rom byte*)"CarrierFreq:                    ",2,4);
	data_ptr=decodedData;
	printToScreen(0,data_ptr,0,15,4);	
*/

/*
void drawFreq(byte firmwareVersion){

    byte j;
    byte k;
	byte byteContainer[5];

	byte* data_ptr;

	word rawData[3];
    byte decodedData[9];

	rawData[0]=ReadMem(F_WORD,dataLocator[firmwareVersion][FREQ_A]);//

	byteContainer[0]=rawData[0];
	rawData[0]>>=8;
	byteContainer[1]|=rawData[0];

	for(j=0; j<2u; j++)
	{
		byteContainer[2]=byteContainer[0];

		byteContainer[2]&=0xF0;

		for(k=0u; k<=15u; k++)
		{
			if(byteCodepad[1][k]==byteContainer[2]){
				decodedData[j]=byteCodepad[0][k];
				break;
			}
		}
		
		byteContainer[0]<<=4;
	}

	decodedData[4]='\0';
			   	//             "<             max-length            >"
	printToScreen(0,0,(rom byte*)"CarrierFreq:                    ",2,4);
	data_ptr=decodedData;
	printToScreen(0,data_ptr,0,15,4);	
}
*/

/*

//below was a part of the early version of the MSP430 ProgD FW. Nowadays iam able to buffer the FW part to
//the local RAM without occupy memory for every SV-FW version.

//because i cannot overwrite a sigle byte in the flash without deleting a whole
//segment before, it is nessasary to backup the whole segment of the FW here.
//the important part (net id, segments an freq) can of course be modified before writing to
//flash. this procedure allows keeping the current FW and just modifying the user
//definable data. no matter what FW was on the component before.

// Backup of WN.RN.ER FW 2009
#pragma idata
//								0x0100,0x0008
const word data1[8]={0xA06A,0x0100,0x0008,0xF008,0xA0DA,0x0004,0x0012,0xA27E};
//								0x0902
const word data2[3]={0x3F79,0x0902,0xFF00};
const word data3[3]={0xA15C,0x0002,0x0012};
const word data4[4]={0xA280,0x3F79,0x0A01,0xFF00};
const word data5[2]={0xA04E,0x0001};
const word data6[4]={0x001A,0x0000,0x0000,0x0901};
const word data7[1]={0xA04E};
const word data8[4]={0x0002,0x001A,0x0C00,0xFF00};
const word data9[1]={0x0A02};
const word data10[4]={0xA04E,0x0003,0x001A,0x0A00};
const word data11[2]={0xFF00,0x0000};
const word data12[3]={0xA05C,0x0001,0x000E};
const word data13[4]={0x0A00,0xFF00,0x0000,0x0000};

// Backup of WN.LN.R FW 2007
//								0x0100,0x0008
const word data14[8]={0xA06A,0x0100,0x0008,0xF008,0xA0DA,0x0004,0x0012,0xA27E};
//								0x0902
const word data15[3]={0x3F79,0x0902,0xFF00};
const word data16[3]={0xA15C,0x0002,0x0012};
const word data17[4]={0xA280,0x3F79,0x0A01,0xFF00};
const word data18[2]={0xA04E,0x0001};
const word data19[4]={0x001A,0x0000,0x0000,0x0901};
const word data20[1]={0xA04E};
const word data21[4]={0x0002,0x001A,0x0C00,0xFF00};
const word data22[1]={0x0A02};
const word data23[4]={0xA04E,0x0003,0x001A,0x0A00};
const word data24[2]={0xFF00,0x0000};
const word data25[3]={0xA05C,0x0001,0x000E};
const word data26[4]={0x0A00,0xFF00,0x0000,0x0000};
*/


// This will store (or backup) up to 1024 bytes of the target firmware.
// This is because i cannot overwrite a sigle byte in the flash without deleting a whole
// segment before, it is nessasary to backup the whole segment of the FW here.
// the important part (net id, segments an freq) can of course be modified before writing to
// flash back. this procedure allows keeping the current FW and just modifying the user
// definable data. no matter what FW was on the component before.
/*
#pragma udata gpr1
 word firmwareData1[128];
 #pragma udata

#pragma udata gpr2
 word firmwareData2[128];
 #pragma udata

#pragma udata gpr3
 word firmwareData3[128];
 #pragma udata

#pragma udata gpr4 // out of 16 registers with 256 bytes each / or 128 words
 word firmwareData4[128];
 #pragma udata
*/