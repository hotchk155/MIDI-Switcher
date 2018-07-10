////////////////////////////////////////////////////////////
//
//   ///////  ///////  ////////  /////////    ////////
//   ////  ////  ////    ////    ////   ////    ////
//   ////  ////  ////    ////    ////   ////    ////
//   ////        ////    ////    ////   ////    ////
//   ////        ////  ////////  /////////    ////////
//
//   ////  //    // // ////// //// //   // ///// /////
//  //     //    // //   //  //    //   // //    //  //
//   ////  // // // //   //  //    /////// ////  /////
//      // // // // //   //  //    //   // //    //  //
//   ////  ///  /// //   //   //// //   // ///// //  //
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
// 1.0 	21Dec2013	Initial version
// 1.1 	15Dec2014	Add support for relay board
// 1.2 	21Mar2015	First release of relay board
//
#define VERSION_MAJOR 1
#define VERSION_MINOR 2
//
////////////////////////////////////////////////////////////

#include <system.h>
#include <memory.h>
#include <eeprom.h>

// for Transistor board comment this line out 
//#define RELAY_SWITCHER 1
//#define TRS_SWITCHER 1
#define SMT_SWITCHER 1



// CONFIG OPTIONS 
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000


/*
Original MIDI Switcher
		
		VDD - VSS
LED		RA5	- RA0/PGD	P7
SW		RA4 - RA1/PGC	P6
		VPP - RA2		P5
RX		RC5 - RC0		P4
P0		RC4 - RC1		P1
P2		RC3 - RC2		P3

Relays Board
		
		VDD - VSS
LED		RA5	- RA0/PGD	P7
SW		RA4 - RA1/PGC	P6
		VPP - RA2		P5
RX		RC5 - RC0		P4
P1		RC4 - RC1		P3
P0		RC3 - RC2		P2
		

TRS Switcher Board
		
		VDD - VSS
SW		RA5	- RA0/PGD	P0
LED		RA4 - RA1/PGC	P1
		VPP - RA2		P5
RX		RC5 - RC0		P4
P7		RC4 - RC1		P3
P6		RC3 - RC2		P2

SMT Switcher Board
		
		VDD - VSS
P0		RA5	- RA0/PGD	
LED		RA4 - RA1/PGC	RX
SW		VPP - RA2		P4
P2		RC5 - RC0		P8
P5		RC4 - RC1		P3
P6		RC3 - RC2		P7
		
*/
#ifdef RELAY_SWITCHER
	#define P_OUT0		latc.3 
	#define P_OUT1		latc.4 
	#define P_OUT2		latc.2
	#define P_OUT3		latc.1 
	#define P_OUT4		latc.0
	#define P_OUT5		lata.2 
	#define P_OUT6		lata.1
	#define P_OUT7		lata.0	

	#define T_OUT0		trisc.3 
	#define T_OUT1		trisc.4 
	#define T_OUT2		trisc.2
	#define T_OUT3		trisc.1 
	#define T_OUT4		trisc.0
	#define T_OUT5		trisa.2 
	#define T_OUT6		trisa.1
	#define T_OUT7		trisa.0	

	#define P_LED		porta.5
	#define P_MODE		porta.4
	
	#define T_LED		trisa.5
	#define T_MODE		trisa.4
	#define T_RX		trisc.5

	#define P_WPU		wpua.4
	
	#define APFCON0_MASK	0
	
#elif TRS_SWITCHER

	#define P_OUT0		lata.0
	#define P_OUT1		lata.1 
	#define P_OUT2		latc.2
	#define P_OUT3		latc.1 
	#define P_OUT4		latc.0
	#define P_OUT5		lata.2 
	#define P_OUT6		latc.3
	#define P_OUT7		latc.4	

	#define T_OUT0		trisa.0
	#define T_OUT1		trisa.1 
	#define T_OUT2		trisc.2
	#define T_OUT3		trisc.1 
	#define T_OUT4		trisc.0
	#define T_OUT5		trisa.2 
	#define T_OUT6		trisc.3
	#define T_OUT7		trisc.4	

	#define P_LED		porta.4
	#define P_MODE		porta.5
	
	#define T_LED		trisa.4
	#define T_MODE		trisa.5
	#define T_RX		trisc.5

	#define P_WPU		wpua.5

	#define APFCON0_MASK	0

#elif SMT_SWITCHER

	#define P_OUT0		lata.5
	#define P_OUT1		latc.5
	#define P_OUT2		latc.1
	#define P_OUT3		lata.2
	#define P_OUT4		latc.4
	#define P_OUT5		latc.3
	#define P_OUT6		latc.2
	#define P_OUT7		latc.0
	
	#define T_OUT0		trisa.5
	#define T_OUT1		trisc.5 
	#define T_OUT2		trisc.1
	#define T_OUT3		trisa.2 
	#define T_OUT4		trisc.4
	#define T_OUT5		trisc.3 
	#define T_OUT6		trisc.2
	#define T_OUT7		trisc.0	
	
	#define P_LED		porta.4
	#define P_MODE		porta.3
	
	#define T_LED		trisa.4
	#define T_MODE		trisa.3
	#define T_RX		trisa.1

	#define P_WPU		wpua.3

	#define APFCON0_MASK	0x80

#else
	#define P_OUT0		portc.4 
	#define P_OUT1		portc.1 
	#define P_OUT2		portc.3
	#define P_OUT3		portc.2 
	#define P_OUT4		portc.0
	#define P_OUT5		porta.2 
	#define P_OUT6		porta.1
	#define P_OUT7		porta.0
	
	#define T_OUT0		trisc.4 
	#define T_OUT1		trisc.1 
	#define T_OUT2		trisc.3
	#define T_OUT3		trisc.2 
	#define T_OUT4		trisc.0
	#define T_OUT5		trisa.2 
	#define T_OUT6		trisa.1
	#define T_OUT7		trisa.0	
	
	#define P_LED		porta.5
	#define P_MODE		porta.4
	
	#define T_LED		trisa.5
	#define T_MODE		trisa.4
	#define T_RX		trisc.5

	#define P_WPU		wpua.4

	#define APFCON0_MASK	0
	
#endif


typedef unsigned char byte;

// Timer settings
volatile byte timerTicked = 0;		// Timer ticked flag (tick once per ms)
#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz
									// using this init scalar means that rollover
									// interrupt fires once per ms
// MIDI receive buffer
#define SZ_RXBUFFER 20
byte rxBuffer[SZ_RXBUFFER];
byte rxHead = 0;
byte rxTail = 0;

// MIDI message read status
byte midiRunningStatus = 0;
byte midiNumParams = 0;
byte midiCurrentParam = 0;
byte midiParams[2] = {0};

// LED status
#define LEDCOUNT_BRIEF 10
#define LEDCOUNT_MEDIUM 100
#define LEDCOUNT_LONG 500

// Special EEPROM data values
#define EEPROM_ADDR_CHECKSUM 		255
#define EEPROM_FLAG_INVERT	 		0x01

// Total number of ports
#define NUM_PORTS 8


// Special MIDI CC numbers
enum {
	MIDI_NRPN_HI 				= 99,
	MIDI_NRPN_LO 				= 98,
	MIDI_DATA_HI 				= 6,
	MIDI_DATA_LO 				= 38
};


// Configuration messages. 
// NRPN MSB = Trigger (1-8)
// NRPN LSB = Config param (from list below)
enum {
	NRPN_LO_TRIGGERCHANNEL 		= 1,		
	NRPN_LO_TRIGGERNOTE 		= 2,
	NRPN_LO_DURATION 			= 3,
	NRPN_LO_DURATIONMODULATOR 	= 4,
	NRPN_LO_DUTY 				= 5,
	NRPN_LO_DUTYMODULATOR 		= 6,
	NRPN_LO_INVERT 				= 7
};

// Data save message
// NRPN MSB = 100
// NRPN LSB = 1
enum {
	NRPN_HI_EEPROM 				= 100,
	NRPN_LO_SAVE 				= 1
};

// Special modulator values. Stored instead of CC number in durationModulator/dutyModulator
enum {
	MODULATOR_NONE 				= 0x80,	// No modulation (0 not used as it is a valid CC number)
	MODULATOR_NOTEVELOCITY 		= 0x81,	// Modulate by note velocity
	MODULATOR_PITCHBEND 		= 0x82	// Modulate by pitchbend
};

// Special duration values
enum {
	WHILE_NOTE_HELD = -1			// Stay triggered until NOTE OFF received
};

/////////////////////////////////////////////////////////////////////
// Structure defines configuration of a port
typedef struct {
	byte triggerChannel;		// MIDI channel on which trigger note is received
	byte triggerNote;			// MIDI note number to trigger this port
	int durationMax;			// Duration in milliseconds (at 100% modulation) of output pulse
	byte durationModulator;		// Modulation source for pulse duration
	byte dutyMax;				// Duty cycle % (at 100% modulation) of output pulse
	byte dutyModulator;			// Modulation source for duty 
	byte invert;				// Whether output is active LOW
} PORT_CONFIG;

/////////////////////////////////////////////////////////////////////
// Structure defines current state of port
typedef struct
{
	int count;					// Remaining milliseconds for port to remain triggered (-1 for unlimited)
	int duration;				// Modulated duration
	byte duty;					// Modulated duty 
	byte durationCCValue;		// Last value for the CC modulating duration
	byte dutyCCValue;			// Last value for the CC modulating duty
	byte velocity;				// Last value for note velocity
	byte pitchbendMSB;			// Last value for pitchbend MSB
} PORT_STATUS;

/////////////////////////////////////////////////////////////////////
// Structure to contain all the information for a port
typedef struct
{
	PORT_CONFIG cfg;
	PORT_STATUS status;
} PORT_INFO;

/////////////////////////////////////////////////////////////////////
// Array of port info (due to limitation on PIC RAM paging these
// cannot be allocated in a contiguous block)
PORT_INFO* port[NUM_PORTS]; 
PORT_INFO port0;
PORT_INFO port1;
PORT_INFO port2;
PORT_INFO port3;
PORT_INFO port4;
PORT_INFO port5;
PORT_INFO port6;
PORT_INFO port7;

// Flag to say config info has been changed but not saved
byte isDirtyConfig = 0;

////////////////////////////////////////////////////////////
// INTERRUPT SERVICE ROUTINE
void interrupt( void )
{
	// TIMER 0 TICK
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		timerTicked = 1;
		intcon.2 = 0;
	}
	
	// SERIAL RECEIVE CHARACTERR
	if(pir1.5)
	{
		// get the byte
		byte b = rcreg;
		
		// calculate next buffer head
		byte nextHead = (rxHead + 1);
		if(nextHead >= SZ_RXBUFFER) 
		{
			nextHead -= SZ_RXBUFFER;
		}
		
		// if buffer is not full
		if(nextHead != rxTail)
		{
			// store the byte
			rxBuffer[rxHead] = b;
			rxHead = nextHead;
		}
	}
}

////////////////////////////////////////////////////////////
// FLASH DIAGNOSTIC LED
void flashLed(int i)
{
	while(i>0)
	{
		P_LED = 1;
		delay_ms(200);
		P_LED = 0;
		delay_ms(200);
		--i;
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
	spbrg = 31;		// brg low byte (31250)		
	
}

////////////////////////////////////////////////////////////
// RECEIVE MIDI MESSAGE FROM BUFFER
// Return the status byte or 0 if nothing complete received
// caller must check midiParams array for byte 1 and 2
byte receiveMessage()
{
	// buffer overrun error?
	if(rcsta.1)
	{
		rcsta.4 = 0;
		rcsta.4 = 1;
	}
	
	while(rxHead != rxTail)
	{	
		// get next byte from buffer
		byte q = rxBuffer[rxTail];
		if(++rxTail >= SZ_RXBUFFER)
			rxTail = 0;
					
		// is it a status msg
		if(q&0x80)
		{
			switch(q&0xf0)
			{
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xA0: //  Aftertouch  2  key  touch  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xC0: //  Patch change  2  instrument #   
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
				midiRunningStatus = q;
				midiNumParams = 2;
				midiCurrentParam = 0;
				break;
			case 0xD0: //  Channel Pressure  1  pressure  
				midiRunningStatus = q;
				midiNumParams = 1;
				midiCurrentParam = 0;
				break;
			case 0xF0: //  (non-musical commands) - ignore all data for now			
				break;
			}
		}
		else
		{
			// store MIDI parameter
			midiParams[midiCurrentParam++] = q;
			if(midiCurrentParam >= midiNumParams)
			{
				// we have all the parameters for a new message
				midiCurrentParam = 0;
				switch(midiRunningStatus & 0xF0)
				{
					// we only need to return one of:
					case 0x80:	// note off
					case 0x90:	// note on
					case 0xB0:	// controller
					case 0xE0:	// pitch bend
						return midiRunningStatus;
				}				
			}
		}
	}
	
	// Cannot return a complete message
	return 0;
}

////////////////////////////////////////////////////////////
// CHECKSUM CALCULATION
byte calcCheckSum(byte *base, int count, byte *cs)
{
	for(int i=0; i<count; ++i)
		*cs = (*cs<<1) ^ base[i];
}

//////////////////////////////////////////////////////////////////////////////////
// SET DEFAULT VALUES FOR PORT
void initPortInfo()
{
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		memset(p, 0, sizeof(PORT_INFO));
		p->cfg.triggerChannel = 0;
		p->cfg.triggerNote = 60 + i;
		p->cfg.durationMax = 20; // default duration
		p->cfg.durationModulator = MODULATOR_NONE;
		p->cfg.dutyMax = 100; // duty
		p->cfg.dutyModulator = MODULATOR_NONE;
		p->cfg.invert = 0;
		
		// full duration/duty
		p->status.duration = p->cfg.durationMax;
		p->status.duty = p->cfg.dutyMax;
	}
}

////////////////////////////////////////////////////////////
// SAVE PORT INFO TO EEPROM
void savePortInfo()
{
	byte cs = 0;
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];		
		calcCheckSum((byte*)&p->cfg, sizeof(PORT_CONFIG), &cs); // include checksum
		
		int eepromAddr = 16 * i; // allocate 16 bytes for each port config
		eeprom_write(eepromAddr+0, p->cfg.triggerChannel);
		eeprom_write(eepromAddr+1, p->cfg.triggerNote);
		eeprom_write(eepromAddr+2, (p->cfg.durationMax >> 8) & 0xFF);
		eeprom_write(eepromAddr+3, p->cfg.durationMax & 0xFF);
		eeprom_write(eepromAddr+4, p->cfg.durationModulator);
		eeprom_write(eepromAddr+5, p->cfg.dutyMax);
		eeprom_write(eepromAddr+6, p->cfg.dutyModulator);
		
		byte flags = 0;
		if(p->cfg.invert) flags |= EEPROM_FLAG_INVERT;
		eeprom_write(eepromAddr+7, flags);
	}
	eeprom_write(EEPROM_ADDR_CHECKSUM, cs);	
}

////////////////////////////////////////////////////////////
// LOAD PORT INFO FROM EEPROM
void loadPortInfo(byte forceReload)
{
	byte cs = 0;
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		memset(p, 0, sizeof(PORT_INFO));
		
		int eepromAddr = 16 * i;
		p->cfg.triggerChannel 	 = eeprom_read(eepromAddr + 0);
		p->cfg.triggerNote 		 = eeprom_read(eepromAddr + 1);
		p->cfg.durationMax		 = (int)eeprom_read(eepromAddr + 2) << 8;
		p->cfg.durationMax	    |= (int)eeprom_read(eepromAddr + 3);
		p->cfg.durationModulator = eeprom_read(eepromAddr + 4);
		p->cfg.dutyMax			 = eeprom_read(eepromAddr + 5);
		p->cfg.dutyModulator	 = eeprom_read(eepromAddr + 6);		
		byte flags = eeprom_read(eepromAddr + 7);
		p->cfg.invert = !!(flags & EEPROM_FLAG_INVERT);
		
		// default to full duration/duty
		p->status.duration = p->cfg.durationMax;
		p->status.duty = p->cfg.dutyMax;
		
		calcCheckSum((byte*)&p->cfg, sizeof(PORT_CONFIG), &cs); 
	}
	
	if(forceReload || eeprom_read(EEPROM_ADDR_CHECKSUM) != cs)
	{
		// failed checksum (possibly no previous stored config)
		flashLed(10);
		initPortInfo();
		savePortInfo();
	}
}

//////////////////////////////////////////////////////////////////////////////////
// HANDLE NRPN INFO FOR PORT CONFIGURATION
byte handleNrpn(PORT_INFO *p, byte nrpnLo, byte data, byte isLSB)
{
	byte recognised = 1;
	switch(nrpnLo)
	{
	case NRPN_LO_TRIGGERCHANNEL:
		if(isLSB)
			p->cfg.triggerChannel = data;
		break;
	case NRPN_LO_TRIGGERNOTE:
		if(isLSB)
			p->cfg.triggerNote = data;
		break;
	case NRPN_LO_DURATION:
		if(isLSB)
		{
			p->cfg.durationMax &= ~0x7F;
			p->cfg.durationMax |= data;
		}
		else
		{
			// we expect to see MSB first
			p->cfg.durationMax = ((int)data)<<7;
		}
		p->status.duration = p->cfg.durationMax;
		break;
	case NRPN_LO_DURATIONMODULATOR:
		if(isLSB)
		{
			p->cfg.durationModulator &= ~0x7F;
			p->cfg.durationModulator |= data;
		}
		else
		{
			p->cfg.durationModulator = ((int)data)<<7;
		}
		p->status.duration = p->cfg.durationMax;
		break;
	case NRPN_LO_DUTY:
		if(isLSB)
		{
			p->cfg.dutyMax = data;
			p->status.duty = p->cfg.dutyMax;
		}
		break;
	case NRPN_LO_DUTYMODULATOR:
		if(isLSB)
		{
			p->cfg.dutyModulator &= ~0x7F;
			p->cfg.dutyModulator |= data;
		}
		else
		{
			p->cfg.dutyModulator = ((int)data)<<7;
		}
		p->status.duty = p->cfg.dutyMax;
		break;
	case NRPN_LO_INVERT:
		if(isLSB)
			p->cfg.invert = data;
		break;
	default:
		recognised = 0;
		break;
	}
	if(recognised)
		isDirtyConfig = 1;
}

#define APPLY_MODULATOR(a,b) ((((long)(a) * (b))>>7)&0x7f)

//////////////////////////////////////////////////////////////////////////////////
// HANDLE MODULATION BY CC
void handleCC(byte chan, byte cc, byte value)
{
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		if(p->cfg.triggerChannel == chan)
		{
			if(p->cfg.durationModulator == cc)
			{
				p->status.duration = APPLY_MODULATOR(p->cfg.durationMax, value);
			}
			if(p->cfg.dutyModulator == cc)
			{
				p->status.duty = APPLY_MODULATOR(p->cfg.dutyMax, value);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
// HANDLE MODULATION BY PITCHBEND
void handlePitchBend(byte chan, byte MSB)
{
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		if(p->cfg.triggerChannel == chan)
		{
			if(p->cfg.durationModulator == MODULATOR_PITCHBEND)
			{
				p->status.duration = APPLY_MODULATOR(p->cfg.durationMax, MSB);
			}
			if(p->cfg.dutyModulator == MODULATOR_PITCHBEND)
			{
				p->status.duty = APPLY_MODULATOR(p->cfg.dutyMax, MSB);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
// HANDLE NOTE ON MESSAGE
void handleNoteOn(byte chan, byte note, byte velocity)
{
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		if(p->cfg.triggerChannel == chan)
		{
			if(p->cfg.durationModulator == MODULATOR_NOTEVELOCITY)
			{
				p->status.duration = APPLY_MODULATOR(p->cfg.durationMax, velocity);
			}
			if(p->cfg.dutyModulator == MODULATOR_NOTEVELOCITY)
			{
				p->status.duty = APPLY_MODULATOR(p->cfg.dutyMax,velocity);

			}
			if(note == p->cfg.triggerNote)
			{
				//if(!p->cfg.durationMax) 
					p->status.count = WHILE_NOTE_HELD;
				//else
					//p->status.count = p->status.duration;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
// HANDLE NOTE OFF MESSAGE
byte handleNoteOff(byte chan, byte note)
{
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		if(p->cfg.triggerChannel == chan && note == p->cfg.triggerNote)
		{
			if(p->status.count == WHILE_NOTE_HELD) 
				p->status.count = 0;
		}
	}
}

////////////////////////////////////////////////////////////
// MAIN
void main()
{ 
	int i;
	
	// osc control / 16MHz / internal
	osccon = 0b01111010;
		
	apfcon0 = APFCON0_MASK;
	
	// configure io. Initially all outputs are 
	// disabled except for the LED and all outputs
	// states are zeroed
	trisa = 0b11111111;
	trisc = 0b11111111;

	T_LED = 0;    
	ansela = 0b00000000;
	anselc = 0b00000000;
	porta =  0b00000000;
	portc =  0b00000000;

	P_WPU = 1; // weak pull up on switch input
	option_reg.7 = 0; // weak pull up enable

	// initialise MIDI comms
	init_usart();
	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE
	
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 4MHz
	// 	prescaled 1/16 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	option_reg.3 = 0; // timer 0 is prescaled
	option_reg.2 = 0; // }
	option_reg.1 = 1; // } 1/16 prescaler
	option_reg.0 = 1; // }
	intcon.5 = 1; 	  // enabled timer 0 interrrupt
	intcon.2 = 0;     // clear interrupt fired flag
	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE

	// Initialise the table of port info pointers
	port[0] = &port0;
	port[1] = &port1;
	port[2] = &port2;
	port[3] = &port3;
	port[4] = &port4;
	port[5] = &port5;
	port[6] = &port6;
	port[7] = &port7;
	
	// allow time for input to settle then load 
	// EEPROM settings, allowing a hold of MODE
	// to restore default settings
	delay_ms(5);
	loadPortInfo((!P_MODE));		

#ifdef RELAY_SWITCHER
	// In relay switching mode set all output latches LOW
	// and set default tristate (INPUT/1 selected for OFF)
	T_OUT0 = !port0.cfg.invert;
	T_OUT1 = !port1.cfg.invert;
	T_OUT2 = !port2.cfg.invert;
	T_OUT3 = !port3.cfg.invert;
	T_OUT4 = !port4.cfg.invert;
	T_OUT5 = !port5.cfg.invert;
	T_OUT6 = !port6.cfg.invert;
	T_OUT7 = !port7.cfg.invert;
		
	P_OUT0 = 0;		P_OUT1 = 0;		P_OUT2 = 0;		P_OUT3 = 0;
	P_OUT4 = 0;		P_OUT5 = 0;		P_OUT6 = 0;		P_OUT7 = 0;
		
#else
	// In transistor switching mode set default "off"
	// states and enable digital outputs
	P_OUT0 = !!port0.cfg.invert;
	P_OUT1 = !!port1.cfg.invert;
	P_OUT2 = !!port2.cfg.invert;
	P_OUT3 = !!port3.cfg.invert;
	P_OUT4 = !!port4.cfg.invert;
	P_OUT5 = !!port5.cfg.invert;
	P_OUT6 = !!port6.cfg.invert;
	P_OUT7 = !!port7.cfg.invert;
		
	T_OUT0 = 0;		T_OUT1 = 0;		T_OUT2 = 0;		T_OUT3 = 0;
	T_OUT4 = 0;		T_OUT5 = 0;		T_OUT6 = 0;		T_OUT7 = 0;
#endif
		
	int ledCount = 0;
	int modeHeld = 0;
	byte enableNrpn = 0;
	byte nrpnLo = 0;
	byte nrpnHi = 0;
	byte pwm=0;
	int cycles = 0;
	for(;;)
	{	
		// Fetch the next MIDI message
		byte msg = receiveMessage();
		if(msg && !ledCount)
			ledCount = LEDCOUNT_BRIEF;
				
		// NOTE ON
		if(((msg & 0xf0) == 0x90) && midiParams[1])
		{
			handleNoteOn(msg&0xF, midiParams[0], midiParams[1]);
		}
		// NOTE OFF
		else if((((msg & 0xf0) == 0x80)||((msg & 0xf0) == 0x90)))
		{
			handleNoteOff(msg&0xF, midiParams[0]);
		}
		// CONTROLLER
		else if((msg & 0xf0) == 0xb0) 
		{			
			if(enableNrpn) 
			{
				switch(midiParams[0])
				{
					case MIDI_NRPN_HI: 
						nrpnHi = midiParams[1]; 
						break;
					case MIDI_NRPN_LO: 
						nrpnLo = midiParams[1]; 
						break;
					case MIDI_DATA_HI: 
					case MIDI_DATA_LO: 
						if(nrpnHi < NUM_PORTS)
							handleNrpn(port[nrpnHi], nrpnLo, midiParams[1], (midiParams[0] == MIDI_DATA_LO));
						else if(nrpnHi == NRPN_HI_EEPROM && nrpnLo == NRPN_LO_SAVE)
							savePortInfo();
					break;
					default:
						handleCC(msg&0xF, midiParams[0], midiParams[1]);
						break;
						
				}
			}
			else
			{
				handleCC(msg&0xF, midiParams[0], midiParams[1]);			
			}
		}
		// PITCH BEND
		else if((msg & 0xf0) == 0xe0) 
		{
			handlePitchBend(msg&0xF, midiParams[0]);
		}
		
		
		if(ledCount)
			P_LED = !!(--ledCount);
			
		// EVERY MS
		if(timerTicked)
		{
			timerTicked = 0;
			
			// Decrement nonzero port latch counts
			for(i=0;i<NUM_PORTS;++i)
			{				
				if(port[i]->status.count>0)
					--port[i]->status.count;
			}
				
			// If the MODE button is held for 1 second
			// this enables the receiving of config info		
			if(!enableNrpn)
			{
				if(P_MODE)//mode button released
				{
					
					modeHeld = 0;
				}
				else if(++modeHeld >= 1000)// mode button pressed for more than 1 second
				{					
					P_LED = 1; delay_s(1); P_LED = 0;
					enableNrpn = 1;
				}
			}
			else if(isDirtyConfig)//config has been updated
			{
				if(!P_MODE)//
				{
					P_LED = 1;	delay_s(1);	P_LED = 0;				
					savePortInfo();
					isDirtyConfig = 0;					
				}
				else
				{
					++cycles;
					if(!(cycles & 0x1FF) && !ledCount)
						ledCount = LEDCOUNT_MEDIUM;
				}
			}
			else
			{
				++cycles;
				if(!(cycles & 0x7FF) && !ledCount)
					ledCount = LEDCOUNT_LONG;
			}
		}

#ifdef RELAY_SWITCHER

		// Manage outputs for relay switcher. PWM is ignored and switching
		// is done on the TRIS bit
		#define OUT_STATE(P) !(P.status.count)
		#define OUT_STATEN(P) !!(P.status.count)
		T_OUT0 = port0.cfg.invert? OUT_STATEN(port0) : OUT_STATE(port0);
		T_OUT1 = port1.cfg.invert? OUT_STATEN(port1) : OUT_STATE(port1);
		T_OUT2 = port2.cfg.invert? OUT_STATEN(port2) : OUT_STATE(port2);
		T_OUT3 = port3.cfg.invert? OUT_STATEN(port3) : OUT_STATE(port3);
		T_OUT4 = port4.cfg.invert? OUT_STATEN(port4) : OUT_STATE(port4);		
		T_OUT5 = port5.cfg.invert? OUT_STATEN(port5) : OUT_STATE(port5);
		T_OUT6 = port6.cfg.invert? OUT_STATEN(port6) : OUT_STATE(port6);
		T_OUT7 = port7.cfg.invert? OUT_STATEN(port7) : OUT_STATE(port7);	
		P_OUT0 = 0;		
		P_OUT1 = 0;		
		P_OUT2 = 0;		
		P_OUT3 = 0;
		P_OUT4 = 0;		
		P_OUT5 = 0;		
		P_OUT6 = 0;		
		P_OUT7 = 0;		
#elif (TRS_SWITCHER || SMT_SWITCHER)
		#define OUT_STATE(P) !!(P.status.count)
		#define OUT_STATEN(P) !(P.status.count)
		P_OUT0 = port0.cfg.invert? OUT_STATEN(port0) : OUT_STATE(port0);
		P_OUT1 = port1.cfg.invert? OUT_STATEN(port1) : OUT_STATE(port1);
		P_OUT2 = port2.cfg.invert? OUT_STATEN(port2) : OUT_STATE(port2);
		P_OUT3 = port3.cfg.invert? OUT_STATEN(port3) : OUT_STATE(port3);
		P_OUT4 = port4.cfg.invert? OUT_STATEN(port4) : OUT_STATE(port4);
		P_OUT5 = port5.cfg.invert? OUT_STATEN(port5) : OUT_STATE(port5);
		P_OUT6 = port6.cfg.invert? OUT_STATEN(port6) : OUT_STATE(port6);
		P_OUT7 = port7.cfg.invert? OUT_STATEN(port7) : OUT_STATE(port7);		
#else		
		// Manage outputs for transistor switcher. PWM is used and switching
		// is done on the digital output bit
		#define OUT_STATE(P) (P.status.count && (pwm < P.status.duty))
		#define OUT_STATEN(P) (!P.status.count && (pwm < P.status.duty))
		P_OUT0 = port0.cfg.invert? OUT_STATEN(port0) : OUT_STATE(port0);
		P_OUT1 = port1.cfg.invert? OUT_STATEN(port1) : OUT_STATE(port1);
		P_OUT2 = port2.cfg.invert? OUT_STATEN(port2) : OUT_STATE(port2);
		P_OUT3 = port3.cfg.invert? OUT_STATEN(port3) : OUT_STATE(port3);
		P_OUT4 = port4.cfg.invert? OUT_STATEN(port4) : OUT_STATE(port4);
		P_OUT5 = port5.cfg.invert? OUT_STATEN(port5) : OUT_STATE(port5);
		P_OUT6 = port6.cfg.invert? OUT_STATEN(port6) : OUT_STATE(port6);
		P_OUT7 = port7.cfg.invert? OUT_STATEN(port7) : OUT_STATE(port7);		
		if(++pwm>100)
			pwm=0;
#endif 
			
	}
}