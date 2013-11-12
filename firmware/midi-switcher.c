//////////////////////////////////////////////////////////////////////////
//
// FOR PIC16F1825
// SOURCEBOOST C
//
//
//////////////////////////////////////////////////////////////////////////
#include <system.h>
#include <memory.h>
#include "midi-switcher.h"

// CONFIG OPTIONS 
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 8000000

#define ROOT_NOTE 30

#define P_OUT0		portc.4 
#define P_OUT1		portc.1 
#define P_OUT2		portc.3
#define P_OUT3		portc.2 
#define P_OUT4		portc.0
#define P_OUT5		porta.2 
#define P_OUT6		porta.1
#define P_OUT7		porta.0

#define P_LED		porta.5

// outputs
//#define P_OUT0		porta.0 
//#define P_OUT1		porta.1 
//#define P_OUT2		porta.2 
//#define P_OUT3		portc.0 
//#define P_OUT4		portc.1
//#define P_OUT5		portc.2 
//#define P_OUT6		portc.3 
//#define P_OUT7		portc.4 

//a0 a1 a2 c0 c1 c2 c3 c4

// Timer 1 used to maintain a millisecond counter
// 8,000,000 fosc
// 2mhz fosc/4
// prescale/8 = 250khz
// 250 = 1ms
// 65536 - 250 = 65286
#define TIMER_INIT_SCALAR 0xff06
#define INIT_TIMER1 { tmr1l=(TIMER_INIT_SCALAR & 0xff); tmr1h=(TIMER_INIT_SCALAR>>8); }
volatile unsigned long millis = 0;

typedef unsigned char byte;

// MIDI defs
#define MIDIMSG_NOTEON	0x90
#define MIDIMSG_NOTEOFF	0x80

// MIDI message registers
byte runningStatus = 0;
int numParams = 0;
byte midiParams[2] = {0};

#define SZ_RXBUFFER 20
byte rxBuffer[SZ_RXBUFFER];
byte rxHead = 0;
byte rxTail = 0;




#define MIDI_NRPN_LO 99
#define MIDI_NRPN_HI 98
#define MIDI_DATA_HI 6
#define MIDI_DATA_LO 38

enum {
	NRPN_HI_PORT1 = 1,
	NRPN_HI_PORT2,
	NRPN_HI_PORT3,
	NRPN_HI_PORT4,
	NRPN_HI_PORT5,
	NRPN_HI_PORT6,
	NRPN_HI_PORT7,
	NRPN_HI_PORT8
};

enum {
	NRPN_LO_COMMIT = 0,
	NRPN_LO_TRIGGERCHANNEL = 1,
	NRPN_LO_TRIGGERNOTE = 2,
	NRPN_LO_DURATION = 3,
	NRPN_LO_DURATIONMODULATOR = 4,
	NRPN_LO_DUTY = 5,
	NRPN_LO_DUTYMODULATOR = 6,
	NRPN_LO_INVERT = 7
};

enum {
	MODULATOR_NONE = 0x80,
	MODULATOR_NOTEVELOCITY = 0x81,
	MODULATOR_PITCHBEND = 0x82
};


typedef struct {
	byte triggerChannel;
	byte triggerNote;
	int durationMax;
	byte durationModulator;
	byte dutyMax;
	byte dutyModulator;
	byte invert;
} PORT_CONFIG;

typedef struct
{
	int count;
	int duration;				// Modulated duration
	byte duty;					// Modulated duty 
	byte durationCCValue;		// Last value for the CC modulating duration
	byte dutyCCValue;			// Last value for the CC modulating duty
	byte velocity;				// Last value for note velocity
	byte pitchbendMSB;			// Last value for pitchbend MSB
} PORT_STATUS;

typedef struct
{
	PORT_CONFIG cfg;
	PORT_STATUS status;
} PORT_INFO;

#define NUM_PORTS 8
PORT_INFO port[NUM_PORTS];

//////////////////////////////////////////////////////////////////////////////////
// Set information for a single port to sensible default values
void initPortInfo(PORT_INFO *p, int sequence)
{
	memset(p, 0, sizeof(PORT_INFO));
	p->cfg.triggerChannel = 0;
	p->cfg.triggerNote = 60 + sequence;
	p->cfg.durationMax = 10;
	p->cfg.durationModulator = MODULATOR_NONE;
	p->cfg.dutyMax = 100;
	p->cfg.dutyModulator = MODULATOR_NONE;
	p->cfg.invert = 0;
	
	p->status.duration = p->cfg.durationMax;
	p->status.duty = p->cfg.dutyMax;
}

//////////////////////////////////////////////////////////////////////////////////
// handle NRPN message directed to specific port config
byte handleNrpn(PORT_INFO *p, byte nrpnLo, byte data, byte isLSB)
{
	int data;
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
		break;
	case NRPN_LO_DUTY:
		if(isLSB)
			pcfg->dutyMax = data;
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
	case NRPN_LO_INVERT:
		if(isLSB)
			pcfg->invert = data;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////
byte handleCC(byte chan, byte cc, byte value)
{
	for(int i=0; i<MAX_PORTS; ++i)
	{
		PORT_INFO *p = &port[i];
		if(p->cfg.triggerChannel == chan)
		{
			if(p->cfg.durationModulator == cc)
				p->status.duration = (byte)(((float)p->cfg.durationMax * value)/127.0);
			if(p->cfg.dutyModulator == cc)
				p->status.duty = (byte)(((float)p->cfg.dutyMax * value)/127.0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
byte handlePitchBend(byte chan, byte MSB, byte LSB)
{
	for(int i=0; i<MAX_PORTS; ++i)
	{
		PORT_INFO *p = &port[i];
		if(p->cfg.triggerChannel == chan)
		{
			if(p->cfg.durationModulator == MODULATOR_PITCHBEND)
				p->status.duration = (byte)(((float)p->cfg.durationMax * MSB)/127.0);
			if(p->cfg.dutyModulator == MODULATOR_PITCHBEND)
				p->status.duty = (byte)(((float)p->cfg.dutyMax * MSB)/127.0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
byte handleNoteOn(byte chan, byte note, byte velocity)
{
	for(int i=0; i<MAX_PORTS; ++i)
	{
		PORT_INFO *p = &port[i];
		if(p->cfg.triggerChannel == chan)
		{
			if(p->cfg.durationModulator == MODULATOR_NOTEVELOCITY)
				p->status.duration = (byte)(((float)p->cfg.durationMax * velocity)/127.0);
			if(p->cfg.dutyModulator == MODULATOR_NOTEVELOCITY)
				p->status.duty = (byte)(((float)p->cfg.dutyMax * velocity)/127.0);
			if(note == p->cfg.triggerNote)
			{
				if(p->cfg.invert)
					p->status.count = 0;
				else if(!p->status.duration) // infinite
					p->status.count = -1;
				else
					p->status.count = p->status.duration;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
byte handleNoteOff(byte chan, byte note)
{
	for(int i=0; i<MAX_PORTS; ++i)
	{
		PORT_INFO *p = &port[i];
		if(p->cfg.triggerChannel == chan && note == p->cfg.triggerNote)
		{
			if(!p->cfg.invert)
				p->status.count = 0;
			else if(!p->status.duration) // infinite
				p->status.count = -1;
			else
				p->status.count = p->status.duration;
		}
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
	spbrg = 15;		// brg low byte (31250)	
	
}

////////////////////////////////////////////////////////////
// INTERRUPT HANDLER CALLED WHEN CHARACTER RECEIVED AT 
// SERIAL PORT
void interrupt( void )
{
	// timer 1 rollover
	if(pir1.0)
	{
		INIT_TIMER1;
		++millis;
		pir1.0 = 0;
		P_LED = 0;
	}
	
	// check if this is serial rx interrupt
	if(pir1.5)
	{

		P_LED = 1;
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
// INCREMENT CIRCULAR BUFFER INDEX
byte rxInc(byte *pbIndex)
{
	// any data in the buffer?
	if((*pbIndex) == rxHead)
		return 0;

	// move to next char
	if(++(*pbIndex) >= SZ_RXBUFFER) 
		(*pbIndex) -= SZ_RXBUFFER;
	return 1;
}

////////////////////////////////////////////////////////////
// RECEIVE MIDI MESSAGE
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
	
	// any data in the buffer?
	if(rxHead == rxTail)
		return 0;
	
	
	// peek at next char in buffer
	byte rxPos = rxTail;					
	byte q = rxBuffer[rxPos];
			
	// is it a channel msg
	if((q&0x80)>0)
	{
		runningStatus = 0;
		switch(q&0xf0)
		{
		case 0x80: //  Note-off  2  key  velocity  
		case 0x90: //  Note-on  2  key  veolcity  
		case 0xA0: //  Aftertouch  2  key  touch  
		case 0xB0: //  Continuous controller  2  controller #  controller value  
		case 0xC0: //  Patch change  2  instrument #   
		case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			runningStatus = q;
			numParams = 2;
			break;
		case 0xD0: //  Channel Pressure  1  pressure  
			runningStatus = q;
			numParams = 1;
			break;
		case 0xF0: //  (non-musical commands) - ignore all data for now			
			return q; 
		}
		// step over the message
		if(!rxInc(&rxPos))
			return 0;

	}
		
	// do we have an active channel message
	if(runningStatus)
	{

		// read params
		for(int thisParam = 0; thisParam < numParams; ++thisParam)
		{				
			midiParams[thisParam] = rxBuffer[rxPos];
			if(!rxInc(&rxPos))
				return 0;
		}
		// commit removal of message
		rxTail = rxPos;
		return runningStatus;
	}
	else
	{
		// remove char from the buffer
		rxInc(&rxTail);
		return q;
	}
	return 0;
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
// MAIN
void main()
{ 
	int i;
	
	// osc control / 8MHz / internal
	osccon = 0b01110010;
	
	// configure io
	trisa = 0b00000000;              	
    trisc = 0b00100000;              
	ansela = 0b00000000;
	anselc = 0b00000000;
	porta=0;
	portc=0;

	// initialise MIDI comms
	init_usart();
	flashLed(3);

	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE
	
	tmr1l = 0;
	tmr1h = 0;
	t1con.5 = 1; // prescaler mode
	t1con.4 = 1; // prescaler mode
	t1con.1 = 0; // internal
	t1con.0 = 1; // enabled
	pie1.0 = 1; // timer 1 interrupt enable
	
	
	
	byte nrpnLo = 0;
	byte nrpnHi = 0;
	byte pwm=0;
	for(;;)
	{	
		// Fetch the next MIDI message
		byte msg = receiveMessage();
		
		// MIDI CONTROL CHANGE
		if((msg & 0xf0) == 0xB0) 
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
						processPortConfig(&port[nrpnHi], nrpnLo, midiParams[1], (midiParams[0] == MIDI_DATA_LO));
					break;
					break;
				default:
					handleCC((msg & 0x0F), midiParams[1], midiParams[2]);
					break;
					
			}
		}
		// NOTE ON
		else if(((msg & 0xf0) == 0x90) && midiParams[1])
		{
			handleNoteOn(msg, midiParams[0], midiParams[1]);
		}
		// NOTE OFF
		else if((((msg & 0xf0) == 0x80)||((msg & 0xf0) == 0x90)))
		{
			handleNoteOff(msg, midiParams[0], midiParams[1]);
		}

		
		
		P_OUT0 = process_output(&outputs[0], pwm);
		P_OUT1 = process_output(&outputs[1], pwm);
		P_OUT2 = process_output(&outputs[2], pwm);
		P_OUT3 = process_output(&outputs[3], pwm);
		P_OUT4 = process_output(&outputs[4], pwm);
		P_OUT5 = process_output(&outputs[5], pwm);
		P_OUT6 = process_output(&outputs[6], pwm);
		P_OUT7 = process_output(&outputs[7], pwm);
		if(++pwm>=128)
			pwm=0;
	}
}