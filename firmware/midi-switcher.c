//////////////////////////////////////////////////////////////////////////
//
// FOR PIC16F1825
// SOURCEBOOST C
//
//
//////////////////////////////////////////////////////////////////////////
#include <system.h>
#include <memory.h>

// CONFIG OPTIONS 
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF

#pragma CLOCK_FREQ 16000000

// Define the GPIOs that are connected to each output
#define P_OUT0		portc.4 
#define P_OUT1		portc.1 
#define P_OUT2		portc.3
#define P_OUT3		portc.2 
#define P_OUT4		portc.0
#define P_OUT5		porta.2 
#define P_OUT6		porta.1
#define P_OUT7		porta.0
#define P_LED		porta.5

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

int ledCount = 0;
#define LEDCOUNT_BRIEF 10
#define LEDCOUNT_MEDIUM 100
#define LEDCOUNT_LONG 500

// Special MIDI CC numbers
enum {
	MIDI_NRPN_HI = 99,
	MIDI_NRPN_LO = 98,
	MIDI_DATA_HI = 6,
	MIDI_DATA_LO = 38
};

// MIDI NRPN LSB used for passing config info
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

// Special modulator CC values
enum {
	MODULATOR_NONE = 0x80,			// No modulation (0 not used as it is a valid CC number)
	MODULATOR_NOTEVELOCITY = 0x81,	// Modulate by note velocity
	MODULATOR_PITCHBEND = 0x82		// Modulate by pitchbend
};

enum {
	WHILE_NOTE_HELD = -1			// Special unlimited duration value
};

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

// Structure to contain all the information for a port
typedef struct
{
	PORT_CONFIG cfg;
	PORT_STATUS status;
} PORT_INFO;

// Total number of ports
#define NUM_PORTS 8

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
// INTERRUPT HANDLER 
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
				return q; 
			}
		}
		else
		{
			midiParams[midiCurrentParam++] = q;
			if(midiCurrentParam >= midiNumParams)
			{
				midiCurrentParam = 0;
				return midiRunningStatus;
			}
		}
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////
// Set information for a ports to sensible default values
void initPortInfo()
{
	port[0] = &port0;
	port[1] = &port1;
	port[2] = &port2;
	port[3] = &port3;
	port[4] = &port4;
	port[5] = &port5;
	port[6] = &port6;
	port[7] = &port7;

	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		memset(p, 0, sizeof(PORT_INFO));
		p->cfg.triggerChannel = 0;
		p->cfg.triggerNote = 60 + i;
		p->cfg.durationMax = 10;
		p->cfg.durationModulator = MODULATOR_NONE;
		p->cfg.dutyMax = 100;
		p->cfg.dutyModulator = MODULATOR_NONE;
		p->cfg.invert = 0;
		
		p->status.duration = p->cfg.durationMax;
		p->status.duty = p->cfg.dutyMax;
	}
}

//////////////////////////////////////////////////////////////////////////////////
// handle NRPN message directed to specific port config
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
		ledCount = LEDCOUNT_MEDIUM;
}

#define APPLY_MODULATOR(a,b) ((((long)(a) * (b))>>7)&0x7f)

//////////////////////////////////////////////////////////////////////////////////
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
				ledCount = LEDCOUNT_BRIEF;				
			}
			if(p->cfg.dutyModulator == cc)
			{
				p->status.duty = APPLY_MODULATOR(p->cfg.dutyMax, value);
				ledCount = LEDCOUNT_BRIEF;				
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
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
				ledCount = LEDCOUNT_BRIEF;				
			}
			if(p->cfg.dutyModulator == MODULATOR_PITCHBEND)
			{
				p->status.duty = APPLY_MODULATOR(p->cfg.dutyMax, MSB);
				ledCount = LEDCOUNT_BRIEF;				
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
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
				ledCount = LEDCOUNT_BRIEF;				
			}
			if(p->cfg.dutyModulator == MODULATOR_NOTEVELOCITY)
			{
				p->status.duty = APPLY_MODULATOR(p->cfg.dutyMax,velocity);
				ledCount = LEDCOUNT_BRIEF;				

			}
			if(note == p->cfg.triggerNote)
			{
				ledCount = LEDCOUNT_BRIEF;				
				if(!p->cfg.durationMax) 
					p->status.count = WHILE_NOTE_HELD;
				else
					p->status.count = p->status.duration;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
byte handleNoteOff(byte chan, byte note)
{
	for(int i=0; i<NUM_PORTS; ++i)
	{
		PORT_INFO *p = port[i];
		if(p->cfg.triggerChannel == chan && note == p->cfg.triggerNote)
		{
			if(p->status.duration == WHILE_NOTE_HELD) 
				p->status.count = 0;
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
// MAIN
void main()
{ 
	int i;
	
	// osc control / 16MHz / internal
	osccon = 0b01111010;
	
	
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
	
	
	initPortInfo();
	
	byte nrpnLo = 0;
	byte nrpnHi = 0;
	byte pwm=0;
	for(;;)
	{	
			
		// Fetch the next MIDI message
		byte msg = receiveMessage();
		
		
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
					break;
				default:
					handleCC(msg&0xF, midiParams[0], midiParams[1]);
					break;
					
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
			for(i=0;i<NUM_PORTS;++i)
			{				
				if(port[i]->status.count>0)
					--port[i]->status.count;
			}
		}
		
		// RUN OUTPUTS
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
	}
}