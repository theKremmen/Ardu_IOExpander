/*
 * plc.cpp
 *
 * Created: 11.1.2017 9:34:45
 * Author: Kremmen
 * Implementation of the Various "relays" - pulse, delay, set/reset etc declared in plc.h

 * This library simulates the elements of a PLC ladder diagram
 * There is no user interface, the user must specify the "ladder" in the main program
 * by creating all the components and then connecting them together
 * using bit and variable numbers (indexes) in the component call arguments.
*/

#include "plc.h"
#include <SPI.h>
#include <TimerOne.h>

#define UINT16_MAX 65535

// Arduino outputs for the shift register strobe latches
#define OE 12		// 595 output strobe/enable
#define STROBE 11	// 165 load/shift strobe

uint8_t bits[BITSPACE];		// allocation for the bit variables
uint16_t ints[INTSPACE];	// allocation for the integer variables

SPISettings spiSettings( SPICLOCK, MSBFIRST, SPI_MODE0 );

uint8_t timerCount = 0;
volatile uint32_t timers[MAXTIMERS];

// Interrupt handler for the PLC timers
void tISR() {
uint8_t cnt;
	for ( cnt = 0; cnt < timerCount; cnt++ ) {
		if ( timers[cnt] > 0 ) timers[cnt]--;
	}
}

// Debug help to list the bit variables (and timers). Not used during normal operation
void listBits() {
uint8_t cnt;
	for ( cnt = 0; cnt < BITSPACE; cnt++ ) {
		if ( cnt % 8  == 0) Serial.println();
		Serial.print(bits[cnt]);
		Serial.print(" ");
	}
	Serial.println();
}
void listTimers() {
	uint8_t cnt;
	Serial.println(timerCount);
	for ( cnt = 0; cnt < MAXTIMERS; cnt++ ) {
		if ( cnt % 8  == 0) Serial.println();
		Serial.print(timers[cnt]);
		Serial.print(" ");
	}
	Serial.println();
}

// Helper function to extract a bit from the bitspace
bool Bit(logicBit bit) {									// Bit interrogation routine
	return bits[ bit / 8 ]	 & (1 << (bit % 8));
};

// Helper function to set/reset a bit in the bitspace
void setBit( logicBit bit, bool state ) {
	state ? bits[bit/8] |= (1<<(bit%8)) : bits[bit/8] &= ~(1<<(bit%8));
}

int16_t Int(uint8_t intIndex) { return ints[intIndex]; }

void setInt( uint8_t intIndex, int16_t Value ) {
	ints[intIndex] = Value;
}

// PLC Component classes:
//-------------------------
// (for comments, see header "plc.h"

ComponentList CList;


Component::Component(logicBit inPut, logicBit outPut) {
	inBit = inPut;
	outBit = outPut;
	state = state_OFF;
	CList.add(this);
}

void Component::execute() {

}



Logic2::Logic2(logicBit inPut, logicBit inPut2, logicBit outPut, logicFunction func):Component(inPut, outPut) {
	inBit2 = inPut2;
	lFun = func;
}

void Logic2::execute() {
	switch ( lFun ) {
		case AND: setBit(outBit, Bit(inBit) && Bit(inBit2)); break;
		case NAND: setBit(outBit, !(Bit(inBit) && Bit(inBit2))); break;
		case OR: setBit(outBit, Bit(inBit) || Bit(inBit2)); break;
		case NOR: setBit(outBit, !(Bit(inBit) || Bit(inBit2))); break;
		case XOR: setBit(outBit, Bit(inBit) ^ Bit(inBit2)); break;
	}
}

Calc2::Calc2(numeric inPut, numeric inPut2, numeric outPut, numericFunction func):Component(inPut, outPut) {
	inBit2 = inPut2;
	nFun = func;
}

void Calc2::execute() {
int32_t tmpint;
	switch ( nFun ) {
		case PLUS: {
			tmpint = ints[inBit] + ints[inBit2];
			if ( tmpint > UINT16_MAX ) ints[outBit] = UINT16_MAX;
			else ints[outBit] = tmpint;
			break;
		}
		case MINUS: {
			tmpint = ints[inBit] - ints[inBit2];
			if ( tmpint < 0 ) ints[outBit] = 0;
			else ints[outBit] = tmpint;
			break;
		}
		case MUL: {
			tmpint = ints[inBit] * ints[inBit2];
			if ( tmpint > UINT16_MAX ) ints[outBit] = UINT16_MAX;
			else ints[outBit] = tmpint;
			break;
		}
		case DIV: {
			ints[outBit] = ints[inBit] / ints[inBit2];
			break;
		}
		case MOD: {
			ints[outBit] = ints[inBit] % ints[inBit2];
		}
	}
}

Bistable::Bistable(logicBit inPut, logicBit inPut2, logicBit outPut):Component(inPut, outPut) {
	inBit2 = inPut2;
}

void Bistable::execute() {
	if ( Bit(inBit) ) setBit( outBit, true );
	else if ( Bit(inBit2) ) setBit( outBit, false );
}

Astable::Astable(logicBit inPut, logicBit outPut, uint32_t Time1, uint32_t Time2):Component(inPut, outPut) {
	onTime = Time1;
	offTime = Time2;
	timerIndex = timerCount++;
}

void Astable::execute() {
bool inp;
uint32_t tmpTimer;
	inp = Bit(inBit);
	if ( inp) {	// enabled, component is active
		if ( !prevInput ) {	// enable turned on just now; initialize cycle
			state = state_ON;
			setBit(outBit, true);
			cli();
			timers[timerIndex] = onTime;
			sei();
		}

		cli();	// get a copy of the timer
		tmpTimer = timers[timerIndex];
		sei();

		if ( tmpTimer == 0 ) {	// if timer expired, flip the cycle on-off-on...
			if ( state == state_ON ) {
				state = state_OFF;
				setBit( outBit, false);
				cli();
				timers[timerIndex] = offTime;
				sei();
			}
			else {
				state = state_ON;
				setBit( outBit, true);
				cli();
				timers[timerIndex] = onTime;
				sei();
			}
		}
	}
	else {	// not enabled, turn off if active
		if ( state == state_ON ) {
			setBit( outBit, false );
			state = state_OFF;
		}
	}
	prevInput = inp;
}

Monostable::Monostable(logicBit inPut, logicBit outPut, uint32_t pulseTime):Component(inPut, outPut) {
	setTime = pulseTime;
	prevInput = false;
	timerIndex = timerCount++;
}

void Monostable::execute() {
uint32_t tmpTimer;
bool inp;
	inp = Bit(inBit);
	if ( inp ) {
		if ( !prevInput ) {	// rising edge
			setBit(outBit, true);
			state = state_ON;
			cli();
			timers[timerIndex] = setTime;
			sei();
		}
	}
	if ( state == state_ON ) {
		cli();
		tmpTimer = timers[timerIndex];
		sei();
		if ( tmpTimer == 0 ) {
			setBit(outBit, false);
			state = state_OFF;
		}
	}
	prevInput = inp;
}

VMonostable::VMonostable(logicBit inPut, logicBit outPut, uint8_t pulseTimeIndex):Component(inPut, outPut) {
	setTimeIndex = pulseTimeIndex;
	prevInput = false;
	timerIndex = timerCount++;
}

void VMonostable::execute() {
	uint32_t tmpTimer;
	bool inp;
	inp = Bit(inBit);
	if ( inp ) {
		if ( !prevInput ) {	// rising edge
			setBit(outBit, true);
			state = state_ON;
			cli();
			timers[timerIndex] = bits[setTimeIndex];
			sei();
		}
	}
	if ( state == state_ON ) {
		cli();
		tmpTimer = timers[timerIndex];
		sei();
		if ( tmpTimer == 0 ) {
			setBit(outBit, false);
			state = state_OFF;
		}
	}
	prevInput = inp;
}

DnCounter::DnCounter(logicBit clock, logicBit reset, logicBit outPut, uint16_t initCount):Component(clock, outPut) {

	inBit2 = reset;
	initialCount = initCount;
	count = initialCount;
	prevInput = false;
}

void DnCounter::execute() {
bool tmpBit;
	tmpBit = Bit(inBit);
	if ( Bit( inBit2 ) ) {
		setBit(outBit, false);
		count = initialCount;
	}
	else {
		if ( tmpBit && !prevInput ) {
			if ( count > 0 ) count--;
			if (count == 0) {
				setBit( outBit, true );
			}
		}
	}
	prevInput = tmpBit;
}

UpCounter::UpCounter(logicBit clock, logicBit reset, numeric outPut):Component(clock, outPut) {
	inBit2 = reset;
	ints[outBit] = 0;
	prevInput = false;
}

void UpCounter::execute() {
bool tmpBit;
	tmpBit = Bit(inBit);
	if ( Bit( inBit2 ) ) {
		ints[outBit] = 0;
	}
	else {
		if ( tmpBit && !prevInput ) {
			if ( ints[outBit] < UINT16_MAX ) ints[outBit]++;
		}
	}
	prevInput = tmpBit;
}


Delay::Delay(logicBit inPut, logicBit inPut2, logicBit outPut, uint32_t delayTime, uint32_t trigTime):Component(inPut, outPut) {
	inBit2 = inPut2;
	setTime_d = delayTime;
	setTime_t = trigTime;
	prevInput = false;
	timerIndex = timerCount++;
}

void Delay::execute() {
uint32_t tmpTimer;
bool inp;
	inp = Bit( inBit2 );
	if ( inp ) {	// reset
			setBit( outBit, false );
			state = state_OFF;
	}
	switch ( state ) {
		case state_OFF:
			inp = Bit(inBit);
			if ( inp &&  !prevInput ) {	// rising edge
				state = state_TIMING;
				cli();
				timers[timerIndex] = setTime_d;
				sei();
			}
			break;
		case state_TIMING:
			cli();
			tmpTimer = timers[timerIndex];
			sei();
			if ( tmpTimer == 0 ) {
				cli();
				timers[timerIndex] = setTime_t;
				sei();
				setBit(outBit, true);
				state = state_ON;
			}
			break;
		case state_ON:
			cli();
			tmpTimer = timers[timerIndex];
			sei();
			if ( ( tmpTimer == 0) && !inp ) {
				setBit( outBit, false );
				state = state_OFF;
			}
			break;
	}
	prevInput = inp;
}

VDelay::VDelay(logicBit inPut, logicBit inPut2, logicBit outPut, uint8_t delayTimeIndex, uint8_t trigTimeIndex):Component(inPut, outPut) {
	inBit2 = inPut2;
	setTime_dIndex = delayTimeIndex;
	setTime_tIndex = trigTimeIndex;
	prevInput = false;
	timerIndex = timerCount++;
}

void VDelay::execute() {
	uint32_t tmpTimer;
	bool inp;
	inp = Bit( inBit2 ); 
	if ( inp ) {	// reset
		setBit( outBit, false );
		state = state_OFF;
	}
	switch ( state ) {
		case state_OFF:
			inp = Bit(inBit);
			if ( inp &&  !prevInput ) {	// rising edge
				state = state_TIMING;
				cli();
				timers[timerIndex] = ints[setTime_dIndex];
				sei();
			}
			break;
		case state_TIMING:
			cli();
			tmpTimer = timers[timerIndex];
			sei();
			if ( tmpTimer == 0 ) {
				cli();
				timers[timerIndex] = ints[setTime_tIndex];
				sei();
				setBit(outBit, true);
				state = state_ON;
			}
			break;	
		case state_ON:
			cli();
			tmpTimer = timers[timerIndex];
			sei();
			if ( ( tmpTimer == 0) && !inp ) {
				setBit( outBit, false );
				state = state_OFF;
			}
			break;
	}		
	prevInput = inp;
}

BitMux2_1::BitMux2_1(logicBit inPut, logicBit inPut2, logicBit selector0, logicBit outPut):Component(inPut, outPut) {
	inBit2 = inPut2;
	sel0 = selector0;
}

void BitMux2_1::execute() {
	if ( Bit( sel0 ) ) setBit( outBit, Bit( inBit2 ) );
	else setBit( outBit, Bit( inBit) );
}

BitMux4_1::BitMux4_1(logicBit inPut, logicBit inPut2, logicBit inPut3, logicBit inPut4, logicBit selector0, logicBit selector1, logicBit outPut):Component(inPut, outPut) {
	inBit2 = inPut2;
	inBit3 = inPut3;
	inBit4 = inPut4;
	sel0 = selector0;
	sel1 = selector1;
}

void BitMux4_1::execute() {
bool s0,s1;
	s0 = Bit( sel0 );
	s1 = Bit( sel1 );
	if ( s1 ) {
		if ( s0 ) setBit( outBit, Bit( inBit4 ) );
		else setBit( outBit, Bit( inBit3) );
	}
	else {
		if ( s0 ) setBit( outBit, Bit( inBit2 ) );
		else setBit( outBit, Bit( inBit) );
	}
}

IntMux2_1::IntMux2_1(uint8_t inPut, uint8_t inPut2, logicBit selector0, uint8_t outPut):Component(selector0, outPut) {
	val0Index = inPut;
	val1Index = inPut2;
}

void IntMux2_1::execute() {
	if ( Bit( inBit ) ) ints[outBit] = ints[val1Index];
	else ints[OUTPUT] = ints[val0Index];
}

IntMux4_1::IntMux4_1(uint8_t inPut, uint8_t inPut2, uint8_t inPut3, uint8_t inPut4, logicBit selector0, logicBit selector1, uint8_t outPut):Component(selector0, outPut) {
	val0Index = inPut;
	val1Index = inPut2;
	val2Index = inPut3;
	val3Index = inPut4;
	sel1 = selector1;	// (inBit is re-used as selector 0)
}

void IntMux4_1::execute() {
	bool s0,s1;
	s0 = Bit( inBit );
	s1 = Bit( sel1 );
	if ( s1 ) {
		if ( s0 ) ints[outBit] = ints[val3Index];
		else ints[outBit] = ints[val2Index];
	}
	else {
		if ( s0 ) ints[outBit] = ints[val1Index];
		else ints[outBit] = ints[val0Index];
	}
}

AnalogIn::AnalogIn(uint8_t inPut, uint8_t outPut, float offset, float multiplier):Component(inPut, outPut) {
	offs = offset * 65536L;
	mul = multiplier * 65536L;
}

void AnalogIn::execute() {
union tmpVal_u {
	int32_t l;
	int16_t s[2];
};
tmpVal_u tmpVal;
	tmpVal.l = analogRead(inBit+18);
	tmpVal.l *= mul;
	tmpVal.l += offs;
	ints[outBit] = tmpVal.s[1];
}

CompareNumeric::CompareNumeric(numeric inPut1, numeric inPut2, logicBit outPut, compareOp cmp):Component(inPut1, outPut) {
	inNum2 = inPut2;
	comp = cmp;
}

void CompareNumeric::execute() {
	switch ( comp ) {
		case LT: setBit( outBit, ints[inBit] < ints[inNum2] ); break;
		case LE: setBit( outBit, ints[inBit] <= ints[inNum2] ); break;
		case EQ: setBit( outBit, ints[inBit] == ints[inNum2] ); break;
		case GE: setBit( outBit, ints[inBit] >= ints[inNum2] ); break;
		case GT: setBit( outBit, ints[inBit] > ints[inNum2] ); break;
		default: setBit( outBit, ints[inBit] == ints[inNum2] );
	}
}


void ComponentList::begin() {
uint8_t cnt;
	index = 0;
	pinMode(OE, OUTPUT);
	pinMode(STROBE, OUTPUT);
	digitalWrite(STROBE, HIGH);
	SPI.begin();
	SPI.beginTransaction(spiSettings);
	SPI.transfer16(0x0000);
	digitalWrite(OE, HIGH);
	digitalWrite(OE, LOW);
	Timer1.initialize(TIMERTICK);
	Timer1.attachInterrupt(tISR);
	for ( cnt = 0; cnt < BITSPACE; cnt++) bits[cnt] = 0;
	for ( cnt = 0; cnt < INTSPACE; cnt++) ints[cnt] = 0;
}

bool ComponentList::add( Component *component ) {
	if ( index < MAXCOMPONENTS ) {
		list[index++] = component;
		return true;
	}
	else return false;
}

void ComponentList::execute() {
uint8_t cnt;
uint16_t tmpint;
	digitalWrite(STROBE, LOW);
	digitalWrite(STROBE, HIGH);
	tmpint = bits[2] | (bits[3]<<8);
	tmpint = SPI.transfer16(tmpint);
#ifdef INVERT_INPUTS
	tmpint = ~tmpint;
#endif
	digitalWrite(OE, HIGH);
	digitalWrite(OE, LOW);
	bits[0] = tmpint & 0xff;
	bits[1] = tmpint >> 8;
	for ( cnt = 0; cnt < index; cnt++ ) {
		list[cnt]->execute();
	}
}

