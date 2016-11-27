#define VERSION_MAJOR 1
#define VERSION_MINOR 0
//
////////////////////////////////////////////////////////////

#include <system.h>
#include <memory.h>
#include <eeprom.h>


// CONFIG OPTIONS 
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000

#define CC_CHAN1	71
#define CC_CHAN2	73
#define CC_CHAN3	72

/*
Original MIDI Switcher
		
		VDD - VSS
LED		RA5	- RA0/PGD	P7
SW		RA4 - RA1/PGC	P6
		VPP - RA2		P5
RX		RC5 - RC0		P4
P0		RC4 - RC1		P1
P2		RC3 - RC2		P3
	
	
	
PWM OUTPUTS


CCP1	RC5(not usable)
CCP2	RA5(not usable)/RC3
CCP3	RA2
CCP4	RC1
		
*/

#define P_LED		porta.5
#define P_MODE		porta.4

					//76543210
#define TRIS_A		0b11011111
#define TRIS_C		0b11111111


#define P_WPU		wpua.4



typedef unsigned char byte;


volatile byte midi_status = 0;
volatile byte midi_param = 0;
volatile byte which_cc = 0;
									
rom char *gamma = {
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 1, 1, 
1, 1, 1, 1, 2, 2, 2, 2, 
3, 3, 3, 3, 4, 4, 5, 5, 
6, 6, 7, 7, 8, 8, 9, 10, 
10, 11, 12, 13, 13, 14, 15, 16, 
17, 18, 19, 20, 21, 22, 24, 25, 
26, 27, 29, 30, 32, 33, 35, 36, 
38, 39, 41, 43, 45, 47, 49, 50, 
52, 55, 57, 59, 61, 63, 66, 68, 
70, 73, 75, 78, 81, 83, 86, 89, 
92, 95, 98, 101, 104, 107, 110, 114, 
117, 120, 124, 127, 131, 135, 138, 142, 
146, 150, 154, 158, 162, 167, 171, 175, 
180, 184, 189, 193, 198, 203, 208, 213, 
218, 223, 228, 233, 239, 244, 249, 255 };

////////////////////////////////////////////////////////////
// INTERRUPT SERVICE ROUTINE
void interrupt( void )
{
	// SERIAL RECEIVE CHARACTERR
	if(pir1.5)
	{
		// get the byte
		byte b = rcreg;
		if(!!(b & 0x80)) {
			midi_status = b;
			midi_param = 1;
		}
		else if(midi_status == 0xB0) {
			P_LED = 1;
			if(midi_param == 1) {
				which_cc = b;
				midi_param = 2;
			}
			else 
			{
				switch(which_cc) {
					case CC_CHAN1: ccpr2l = gamma[b]; break;
					case CC_CHAN2: ccpr3l = gamma[b]; break;
					case CC_CHAN3: ccpr4l = gamma[b]; break;
				}
				midi_param = 1;
			}
		}
		pir1.5 = 0;
	}
}


////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void init_usart()
{
	pir1.1 = 1;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 0;		//TXIE 		no interrupts
	pie1.5 = 1;		//RCIE 		enable
	
	baudcon.4 = 0;	// SCKP		synchronous bit polarity 
	baudcon.3 = 1;	// BRG16	enable 16 bit brg
	baudcon.1 = 0;	// WUE		wake up enable off
	baudcon.0 = 0;	// ABDEN	auto baud detect
		
	txsta.6 = 0;	// TX9		8 bit transmission
	txsta.5 = 0;	// TXEN		transmit enable
	txsta.4 = 0;	// SYNC		async mode
	txsta.3 = 0;	// SEDNB	break character
	txsta.2 = 0;	// BRGH		high baudrate 
	txsta.0 = 0;	// TX9D		bit 9

	rcsta.7 = 1;	// SPEN 	serial port enable
	rcsta.6 = 0;	// RX9 		8 bit operation
	rcsta.5 = 1;	// SREN 	enable receiver
	rcsta.4 = 1;	// CREN 	continuous receive enable
		
	spbrgh = 0;		// brg high byte
	spbrg = 30;		// brg low byte (31250)		
	
}


////////////////////////////////////////////////////////////
// MAIN
void main()
{ 
	int i;
	
	// osc control / 16MHz / internal
	osccon = 0b01111010;
		
	trisa = TRIS_A;
	trisc = TRIS_C;

	ansela = 0b00000000;
	anselc = 0b00000000;

	porta = 0;
	portc = 0;
	
	P_WPU = 1; // weak pull up on switch input
	option_reg.7 = 0; // weak pull up enable


	// initialise MIDI comms
	init_usart();
	
	// Configure timer 0 (controls pwm clock)
	// 	timer 0 runs at 4MHz
	//option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	//option_reg.3 = 1; // } prescalar
	//option_reg.2 = 0; // }
	//option_reg.1 = 0; // } 
	//option_reg.0 = 0; // }
	//intcon.5 = 1; 	  // enabled timer 0 interrrupt
	//intcon.2 = 0;     // clear interrupt fired flag

	//pr2=255;

	

	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE	

/*CCP4	RC1*/

/*
CCP1	RC5(not usable)
CCP2	RC3
CCP3	RA2
CCP4	RC1

*/

	// ensure the output drivers for each
	// of the CCPx outputs are disabled
	trisa.2 = 1;
	trisc.1 = 1; 
	trisc.3 = 1; 	
		
	// Set CCP2, CCP3, CCP4 to standard PWM mode
	ccp2con = 0b00001100; 
	ccp3con = 0b00001100; 
	ccp4con = 0b00001100; 

	// zero all duty cycles
	ccpr2l = 30; 
	ccpr3l = 30; 
	ccpr4l = 30; 
	
	// set each CCP module to use timer 2
	ccptmrs = 0b00000000;
	
	// Configure timer 2 for x16 prescaler
	t2con = 0b00000010;

	// load timer 2 period register for 255 duty cycles	
	pr2 = 0xFF; 
	
	// clear Timer2 interrupt flag
	pir1.1 = 0;
	
	// start up the timer
	t2con.2 = 1;	
	
	// wait for Timer2 to overflow once
	while(!pir1.1); 

	// now we can enable the output drivers
	trisa.2 = 0;
	trisc.1 = 0; 
	trisc.3 = 0; 	


	while(1);

}