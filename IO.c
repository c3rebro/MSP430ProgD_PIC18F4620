/*==========================================================================*\  
|                                                                            |  
| IO.c                                                                       |  
|                                                                            |  
| contains: procedures for display initialisation, output and the keypad     |  
|----------------------------------------------------------------------------|  
| Project:              MSP430 ProgD                                         | 
| Developed using:      Microchip MPLAB IDE v8.66 + HITECHC Toolchain 9.83   |   
|----------------------------------------------------------------------------|  
| Author:               STR                                                  |  
| Version:              1.0                                                  |  
| Initial Version:      08-18-12                                             |  
| Last Change:          08-18-12                                             |  
|----------------------------------------------------------------------------|  
| Version history:                                                           |  
| 1.0 08/12 STR         Initial version.                                     |    
|----------------------------------------------------------------------------|  
| Designed 2012 by Steven Rott Germany                                       |  
\*==========================================================================*/   

#include "p18f4620.h"
#include "JTAGfunc.h"   
#include "LowLevelfunc.h"
#include "IO.h" 

const byte keypad[3][16]=
{
'F','B','0','A','E','3','2','1','D','6','5','4','C','9','8','7',
0x70,0x71,0x72,0x73,0xB0,0xB1,0xB2,0xB3,0xD0,0xD1,0xD2,0xD3,0xE0,0xE1,0xE2,0xE3,
0x0F,0x0B,0x00,0x0A,0x0E,0x03,0x02,0x01,0x0D,0x06,0x05,0x04,0x0C,0x09,0x08,0x07
};


word command=0x0010;
byte i=0;
byte *adr;

/*---------------------------------------------------------------------------  
    Puts a string onto the specified location of the display. Due to the fact
    that PIC18F has different data busses for data and program memory and
    initialised strings are always declared as rom (program memory), this
    function has to distiguish between a rom or ram character to print.

    Arguments: pointer to the (rom or ram )string, coordinates on the screen  
    Result: none
*/

void printToScreen(byte singleChar, byte *stringVar, rom byte *stringConst, byte x, byte y)
{
        byte hadr;
        byte ladr;
		word temp;

		//temp=x+(y*40);
		temp=y;
		temp=(temp*40)+x;
		hadr=(temp&0xFF00)>>8;
		ladr=temp&0x00FF;
		
		display(2,ladr,hadr,0x24);		//set Adress Pointer
		display(0,0,0,0xb0);

		if(stringConst)
        {
            while(*stringConst!='\0')			//print a whole line
    		{
    			display(3,0,0,*stringConst);  	//print data and inc adr pointer
    			stringConst++;
    		}
        }

        else if(stringVar)
        {
            while(*stringVar!='\0')			//print a whole line
    		{
    			display(3,0,0,*stringVar);  	//print data and inc adr pointer
    			stringVar++;
    		}
        }

		else{
			display(3,0,0,singleChar);  	//print data and inc adr pointer
		}

		display(0,0,0,0xb2);
}
/*---------------------------------------------------------------------------  
   initialize T6963 display controller and draws the frame of main menu
   Arguments: none
   Result: none
*/

void displayInit(void)
{
	byte i=20;
    byte j=40; 

	display(0,0,0,0x81);		    // Mode Set (80=OR Mode)
	display(2,0,0,0x40);		    // Text Home Adress
	display(2,0x28,0,0x41); 	    // Text Area Set
//	display(2,0x00,0x00,0x42);      // Graphic Home Set
//	display(2,0x15,0,0x43); 	    // Graphic Area Set
	display(2,0,0,0x24);		    // Adress Pointer
	display(2,37,15,0x21);		    // Cursor Pointer (x,y)
	display(2,0,0,0xa7);		    // Cursor Pattern
	display(0,0,0,0xb0);		    // Data Auto Write ON

	while(i--)
	{
		while(j--)
			display(3,0,0,0x20);    // Clear Display RAM 
	}

	display(0,0,0,0xb2);		    // Data Auto Write OFF
	display(0,0,0,0x97);		    // Display Mode (cmd,0 	
								    // cmd(94)=text on, graphic off
								    // cmd(98)=text off, graphic on
								    // cmd(9C)=text on, graphic on)
	display(2,0,0,0x24);
	display(0,0,0,0xb0);

	// draw a frame
	printToScreen(0,0,(rom byte*)"*---------- MSP430 ProgD --------------*",0,0);

	for(i=1u;i<=14u;i++)
		printToScreen(0,0,(rom byte*)"|                                      |",0,i);

	printToScreen(0,0,(rom byte*)"*-v0.01--------------------------------*",0,15);

	printToScreen(0,0,(rom byte*)"cmd:     ",29,15);

	printToScreen(0,0,(rom byte*)">010: applications                    ",1,1);
	printToScreen(0,0,(rom byte*)">020: settings                        ",1,2);
	printToScreen(0,0,(rom byte*)"                                      ",1,3);
	printToScreen(0,0,(rom byte*)">030: help                            ",1,4);
	printToScreen(0,0,(rom byte*)"                                      ",1,6);

	display(0,0,0,0xb2);
}

/*---------------------------------------------------------------------------  
   low level procedure for putting data to T6963 display controller
   Arguments: inf(1byte code, 2byte code or just data)
   Result: none
*/
void display(byte inf, byte dat0, byte dat1, byte cmd_dat)
{

	LATA|=0x0F;     //shadowPORTA;
	LATB=0xFF;     //shadowPORTB;

	ClrCS();

	switch(inf)									//communication protocol for the display controller
		{
			case 0:								//send code with automatic adress pointer
			{
				getDisplayStatus();
				LATB=cmd_dat;				 	//Latch Data to WR Latch
				SetCD(); ClrWR(); SetWR();		//wr to lcd (PORTB)
			}break;
	
			case 1:			 					//send  single byte code plus single command
			{
				getDisplayStatus();
				LATB=dat0;
				ClrCD(); ClrWR();  SetWR();
				getDisplayStatus();
				SetCD();					
				LATB=cmd_dat;
				SetCD(); ClrWR(); SetWR(); 
			}break; 
	
			case 2:								//send 2 byte data plus single command
			{
				getDisplayStatus();
				LATB=dat0;
				ClrCD(); ClrWR(); SetWR();
				getDisplayStatus();
				SetCD();
				LATB=dat1;
				ClrCD(); ClrWR(); SetWR();
				getDisplayStatus();
				SetCD(); 
				LATB=cmd_dat;
				SetCD(); ClrWR(); SetWR();
			}break;
	
			case 3:								//send data without command
			{
				getDisplayStatus();
				transl;
				LATB=cmd_dat;
				ClrCD(); ClrWR(); SetWR();
				getDisplayStatus();
				SetCD();									
			}break;	
			
		}
		SetCS();
}

void cls(void){

	// clear the screen

	for(i=1u;i<=14u;i++)
		printToScreen(0,0,(rom byte*)"|                                      |",0,i);


}
/*---------------------------------------------------------------------------  
   waits for T6963 to be ready 
   Arguments: none
   Result: none
*/
void getDisplayStatus(void)
{
	byte temp=0;										
	LATB=0xFF;
	LATA|=0x0F;

	ClrCS();
	TRISB=0xFF;

	SetWR();
	SetCD();
	ClrRD();
	usDelay(5);
	while(temp!=0x03u)
	{
		temp=PORTB;
		temp&=0x03u;
	}
	SetRD();
	TRISB=0x00;

return;
}

/*---------------------------------------------------------------------------  
   reads a key from keypad (4x4) column is driven by 74xx138
   Arguments: none
   Result: 0xFF if no key is pressed, index of pressed key otherwise
           see IO.h for the decoding
*/

byte waitForSingleKey(byte dataSel)
{
	byte n=0x00;        // KEYPAD columns (4x4) connected via 74xx138      
                        // lines selected by PORTD D0 + D1 (1111 1100 = 0xFC)
                        // PORTD.7 .. PORTD.4 KEYPAD row input
	byte m=15;
	byte result=0x00;
	byte temp;

	LATD=0xFF;

	do
	{
		LATD=0b00001100+n;
		//MsDelay(10);
		result=PORTD;
		temp=result;
		result|=0x0F;

		if(result!=0xFFu)
		{
			do
			{
				temp&=0b11110011;       //PORTD.2 and PORTD.3 dont care
				if(keypad[1][m]==temp){
					switch (dataSel){

					case HEX:
						return(keypad[2][m]);
					break;

					case ASCII:
						return(keypad[0][m]);
					break;
					}
				}
			}while(m--);
		}
		n+=1;
		
	}while(n<=0x04u);
	// create the following bitmask:
	// xxxx xx00
	// xxxx xx01
	// xxxx xx10
	// xxxx xx11
	// so each run selects a different column of the keypad with PortD.0 and PortD.1
    // connected to line A and B of the 74xx138

return(0xFF);
}

word getWholeCommand(void)
{
        byte keyStroke=0xFF;
        byte key[2];

		keyStroke=waitForSingleKey(HEX);

		if(keyStroke!=0xFF)
		{
			command|=keyStroke;
			command<<=4;

			while(waitForSingleKey(HEX)!=0xFF)	// SW debounce and wait until switch release
		    	msDelay(100);

			key[0]=hex2ascii(keyStroke);
			key[1]='\0';
			adr=key;
			//adr++;
			//*adr='\0';
			//adr--;
			printToScreen(0,adr,0,33+i,15);
			keyStroke=0xFF;
			i++;	
		}

		if((i==3u)&&(keyStroke==0xFF)) //(i==3)&&(keyStroke==0xFF)
		{
			keyStroke=0;
			msDelay(500);
			printToScreen(0,0,(rom byte*)"cmd:    ",29,15);
			//command=0x0000;
			i=0;
        return command;			
		}
		keyStroke=0xFF;
}

void txOut(byte character, rom byte *stringConst)
{
    if(!character)
    {
    	while(*stringConst!='\0')
		{
    		TXREG=*stringConst;
    		stringConst++;
			while(!TXSTAbits.TRMT);
		}
    }
    else
   	{
		TXREG=character;
        while(!TXSTAbits.TRMT);
    }
}


void sramWrite(word adress, byte data)
{
	word adressbuffer;

	TRISB=0x00;
	ClrLAL();
	ClrHAL();
	SetAEN();

	adressbuffer=adress;

	LATB=adressbuffer;

	SetLAL();
	ClrLAL();

	adressbuffer>>=8;
	LATB=adressbuffer;
	SetHAL();
	ClrHAL();
	ClrSRAMSEL();
	ClrAEN();
	LATB=data;
	ClrWR();
	SetWR();
	SetSRAMSEL();
	SetAEN();	
}

byte sramRead(word adress)
{
	word adressbuffer;
	byte data;

	ClrLAL();
	ClrHAL();
	SetAEN();

	adressbuffer=adress;

	LATB=adressbuffer;
	SetLAL();
	ClrLAL();

	adressbuffer>>=8;
	LATB=adressbuffer;
	SetHAL();
	ClrHAL();
	ClrSRAMSEL();
	ClrAEN();
	TRISB=0xff;

	ClrRD();
	data=PORTB;
	SetRD();
	TRISB=0x00;

	SetSRAMSEL();
	SetAEN();

	return data;	
}
/*---------------------------------------------------------------------------  
   mirrors a byte... keep it, just in case... :-)        
   Arguments:input byte
   Result: output byte (mirrored)
*/

/*
byte mirror(byte n )
{
	n = ((n >> 1) & 0x55) | ((n << 1) & 0xaa);
	n = ((n >> 2) & 0x33) | ((n << 2) & 0xcc);
	n = ((n >> 4) & 0x0f) | ((n << 4) & 0xf0);
	return n;
}
*/
