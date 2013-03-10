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
#define NUM_OUTPUTS 8
byte rxBuffer[SZ_RXBUFFER];
byte rxHead = 0;
byte rxTail = 0;
int tmr[NUM_OUTPUTS] = {0};


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
	
	
	memset(tmr,0,sizeof(tmr));
	// initialise app variables
	for(;;)
	{
		byte msg = receiveMessage();
		if(((msg & 0xf0) == 0x90) && midiParams[1])
		{			
			if((midiParams[0] >= ROOT_NOTE) && (midiParams[0] < ROOT_NOTE+8))
				tmr[midiParams[0] - ROOT_NOTE] = 50;
		}
		for(int i=0; i<8; ++i)
			if(tmr[i]) --tmr[i];

		P_OUT0 = (tmr[0] > 0);
		P_OUT1 = (tmr[1] > 0);
		P_OUT2 = (tmr[2] > 0);
		P_OUT3 = (tmr[3] > 0);
		P_OUT4 = (tmr[4] > 0);
		P_OUT5 = (tmr[5] > 0);
		P_OUT6 = (tmr[6] > 0);
		P_OUT7 = (tmr[7] > 0);
		delay_ms(1);
	}
}