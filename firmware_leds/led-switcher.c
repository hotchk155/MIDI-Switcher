////////////////////////////////////////////////////////////
//
//   ////        ///////////  /////////    
//   ////        ////         ////   ////  
//   ////        ////////     ////   ////  
//   ////        ////         ////   ////  
//   /////////// ///////////  /////////    
//
//   ////  //    // // ////// //// //   // ///// /////
//  //     //    // //   //  //    //   // //    //  //
//   ////  // // // //   //  //    /////// ////  /////
//      // // // // //   //  //    //   // //    //  //
//   ////  ///  /// //   //   //// //   // ///// //  //
//
// Special firmware build for Original MIDI Switcher
// hardware. Provides three channels of 1kHz PWM 
// control for LEDs etc
//
// The LEDs are mapped as follows
//
// CHANNEL 1 (RED)   - 
// CHANNEL 2 (GREEN) - 
// CHANNEL 3 (BLUE)  - 
// 
// This work is licensed under the Creative Commons 
// Attribution-NonCommercial 3.0 Unported License. 
// To view a copy of this license, please visit:
// http://creativecommons.org/licenses/by-nc/3.0/
//
// Please contact me directly if you'd like a CC 
// license allowing use for commercial purposes:
// jason_hotchkiss<at>hotmail.com
//
// Full repository with hardware information:
// https://github.com/hotchk155/MIDI-Switcher
//
// Ver Date 
// 0.1 	17Dec2016	Initial version
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

// Defiine MIDI CCs to control the three channels
#define CC_CHAN1	71
#define CC_CHAN2	73
#define CC_CHAN3	72

/*
Original MIDI Switcher outputs
		
		VDD - VSS
LED		RA5	- RA0/PGD	P7
SW		RA4 - RA1/PGC	P6
		VPP - RA2		P5
RX		RC5 - RC0		P4
P0		RC4 - RC1		P1
P2		RC3 - RC2		P3
		
	
PIC PWM RESOURCES

CCP1	RC5(not usable)
CCP2	RA5(not usable)/RC3
CCP3	RA2
CCP4	RC1
		
*/

//
// MACRO DEFS
//
#define P_LED		porta.5
#define P_MODE		porta.4

					//76543210
#define TRIS_A		0b11011111
#define TRIS_C		0b11111111
#define P_WPU		wpua.4

#define ACTIVITY_LED_ON_TIME	10 // ms

//
// TYPE DEFS
//
typedef unsigned char byte;

//
// GLOBAL DATA
//
volatile byte midi_status = 0;
volatile byte midi_param = 0;
volatile byte which_cc = 0;
volatile byte led_on = 0;
				
// Gamma correction table - map 7 bit CC value to 
// 8 bit PWM brightness level									
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
		if(!!(b & 0x80)) { // it is a MIDI status byte			
			midi_status = b;
			midi_param = 1;
		}
		else if(midi_status == 0xB0) { // it is a MIDI CC on MIDI channel 1 			
		
			// turn activity LED on
			P_LED = 1;
			led_on = ACTIVITY_LED_ON_TIME; 
			
			// first parameter is CC number - just store it until
			// we get param 2 (CC value)
			if(midi_param == 1) {
				which_cc = b;
				midi_param = 2;
			}
			else 
			{
				// MIDI param 2 - now we can check which CC we are 
				// working with and assign it
				switch(which_cc) {
					case CC_CHAN1: ccpr2l = gamma[b]; break;
					case CC_CHAN2: ccpr3l = gamma[b]; break;
					case CC_CHAN3: ccpr4l = gamma[b]; break;
				}
				
				// back to param 1 
				midi_param = 1;
			}
		}
		
		// interrupt handled
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
	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE	

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
	ccpr2l = 0; 
	ccpr3l = 0; 
	ccpr4l = 0; 
	
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

	// main app loop
	while(1) {
	
		// time the activity LED on period
		delay_ms(1);
		if(led_on) {
			if(--led_on == 0) 
				P_LED = 0;
		}
		
		// if the MODE button is pressed, turn all the channels off
		if(!P_MODE) {
			ccpr2l = 0;
			ccpr3l = 0;
			ccpr4l = 0;
		}
	}
}

//
// END
//