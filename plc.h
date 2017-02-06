/*
 * plc.h
 *
 * Created: 11.1.2017 9:34:45
 * Author: Kremmen
 * Simple logic controller
 * For performance reasons no runtime checking is done so configure with care!
 */ 

// Various "relays" - pulse, delay, set/reset etc

// This library simulates the elements of a PLC ladder diagram
// There is no user interface, the user must specify the "ladder" in the main program
// by creating all the components and then connecting them together
// using bit and variable numbers (indexes) in the component call arguments.


#ifndef _PLC_h
#define _PLC_h

#include "arduino.h"
#include "plcconfig.h"

enum lState {state_OFF=0, state_ON, state_TIMING};	// the internal state of some components
enum logicFunction {AND, NAND, OR, NOR, XOR};		// functions the Logic2 component knows how to do
enum numericFunction {PLUS, MINUS, MUL, DIV, MOD};	// functions the Calc2 component knows how to do
enum compareOp {LT, LE, EQ, GE, GT};				// functions the numeric compare knows how to do

void tISR();										// Timer 1 interrupt routine declaration

bool Bit(logicBit bit);								// Bit interrogation 
void setBit( logicBit bit, bool state );			// Bit set/reset routine

int16_t Int(uint8_t intIndex);						// Integer interrogation routine
void setInt( uint8_t intIndex, int16_t Value );		// Integer set routine

void listBits();									// Debug help to list bit space (as hex so you need to decode that in your head)
void listTimers();									// Debug help to list timers

class ComponentList;								// Advance declaration of Component iterator class

// Base class of all ladder logic components.
// Every real component is derived from this class
// Do NOT attempt to create instances of this class
class Component {
	friend class ComponentList;
public:
	Component(logicBit inPut, logicBit outPut);
protected:
	logicBit inBit, outBit;
	virtual void execute();
	lState state;
};

// Not: Inverts the input bit.
class Not: public Component {
public:
	Not(logicBit inPut, logicBit outPut):Component(inPut,outPut){};
private:
	void execute() { setBit( outBit, !Bit(inBit) ); };
};

// Logic2: A logical function of 2 inputs (outPut = inPut1 <lFun> inPut2)
// You define the logical operation (function) to perform in the call argument
// The argument can be one of AND, NAND, OR, NOR, XOR
class Logic2: public Component {
public:
	Logic2(logicBit inPut1, logicBit inPut2, logicBit outPut, logicFunction func);
private:
	logicBit inBit2;
	logicFunction lFun;
	void execute();
};

// Calc2: A numeric calculation of 2 inputs (outPut = inPut1 <nFun> inPut2)
// You define the numeric function in the call argument
// It can be one of PLUS, MINUS, MUL, DIUV, MOD (modulo)
// The functions saturate either to zero (0) or UINT16_MAX (65535).
// The result cannot be negative!
class Calc2: public Component {
public:
	Calc2(numeric inPut1, numeric inPut2, numeric outPut, numericFunction func);
	private:
	numeric inBit2;
	numericFunction nFun;
	void execute();
};

// Bistable: (Set/Reset latch). InPut = SET, InPut2 = RESET. Set has priority
class Bistable: public Component {
public:
	Bistable(logicBit inPut1, logicBit inPut2, logicBit outPut);
private:
	logicBit inBit2;
	void execute();
};

// Astable: An oscillator producing a "clock" signal with configurable duty cycle
// Time 1 is "ON" time, time 2 is "OFF" time
// inPut is ENABLE, when '1' the astable will go, when '0' it will be "OFF"
// Enabling the astable will start counting from beginning, disabling will reset all timers
// The time values are constants given in Timer1 cycles.
class Astable: public Component {
public:
	Astable(logicBit inPut, logicBit outPut, uint32_t Time1, uint32_t Time2);
private:
	uint32_t onTime, offTime;
	uint8_t timerIndex;
	bool prevInput;
	void execute();
};

// Monostable: Timed pulse on rising edge of input. Time is a constant
// Output completes cycle even if input goes low earlier
class Monostable: public Component {
public:
	Monostable(logicBit trigger, logicBit outPut, uint32_t pulseTime);
private:
	uint32_t setTime;
	uint8_t timerIndex;
	bool prevInput;
	void execute();
};

// VMonostable: Timed pulse on rising edge of input. Time is a variable
// Output completes cycle even if input goes low earlier
class VMonostable: public Component {
public:
	VMonostable(logicBit trigger, logicBit outPut, numeric pulseTimeIndex);
private:
	uint8_t setTimeIndex;
	uint8_t timerIndex;
	bool prevInput;
	void execute();
};

// DnCounter: Down counter. Counts clock edges down from the initial value
// When the counter reaches 0, terminal count goes true.
// Reset input will clear Terminal count and set the counter to the initial value,
// The counter will not count while Reset is asserted
class DnCounter: public Component {
public:
	DnCounter(logicBit clock, logicBit reset, logicBit outPut, uint16_t initCount);
private:
	logicBit inBit2;
	uint8_t initialCount;
	uint16_t count;
	bool prevInput;
	void execute();
};

// UpCounter: Up counter. Counts positive clock edges from 0 upwards.
// The current count is kept in the output numeric variable.
// Reset will immediately set the count to 0 regardless of the state of the trigger
// While reset is asserted the counter will not count.
// The count will saturate at UINT16_MAX, i.e. 65535.
class UpCounter: public Component {
public:
	UpCounter(logicBit clock, logicBit reset, numeric outPut);
private:
	logicBit inBit2;
	numeric countIndex;
	bool prevInput;
	void execute();
};

// Delay: a constant length pulse delayed by a constant time from the trigger.
// The timing constants are 32 bit unsigned integers.
// Delay is triggered on the rising edge of the inPut signal.
// If input goes low before completion, the pulse is completed regardless.
// InPut2 is RESET (active high) and causes immediate reset of the component to initial state.
class Delay: public Component {
public:
	Delay(logicBit trigger, logicBit reset, logicBit outPut, uint32_t delayTime, uint32_t trigTime);
private:
	logicBit inBit2;
	uint32_t setTime_d;
	uint32_t setTime_t;
	uint8_t timerIndex;
	bool prevInput;
	void execute();

};

// VDelay: Variable delay. Works as Delay, but the time values are program variables (referenced by the index)
// The timing variables are 16 bit unsigned so maximum times are 65535 clock counts.
class VDelay: public Component {
public:
	VDelay(logicBit trigger, logicBit reset, logicBit outPut, numeric delayTimeIndex, numeric trigTimeIndex);
private:
	logicBit inBit2;
	uint8_t setTime_dIndex;
	uint8_t setTime_tIndex;
	uint8_t timerIndex;
	bool prevInput;
	void execute();

};

// BitMux2_1: A 2 to 1 selector. Selects one of the input bits to output based on the state of the selector
class BitMux2_1: public Component {
public:
	BitMux2_1(logicBit inPut1, logicBit inPut2, logicBit selector0, logicBit outPut);
private:
	logicBit inBit2;
	logicBit sel0;
	void execute();
};

// BitMux4_1: A 4 to 1 selector. Works the same as 2_1, only this one has 4 inputs and 2 selectors
class BitMux4_1: public Component {
public:
	BitMux4_1(logicBit inPut1, logicBit inPut2, logicBit inPut3, logicBit inPut4, logicBit selector0, logicBit selector1, logicBit outPut);
private:
	logicBit inBit2, inBit3, inBit4;
	logicBit sel0, sel1;
	void execute();
};

// IntMux2_1: Integer (numeric) 2 to 1 selector. Works the same as the bit components, but this one has numeric inputs (indexes to the numeric variable memory)
class IntMux2_1: public Component {
public:
	IntMux2_1(numeric inPut1, numeric inPut2, logicBit selector0, numeric outPut);
private:
	numeric val0Index, val1Index;
	void execute();
};

// IntMux4_1: Integer 4 to 1 selector. Works the same as above but for 4 inputs and 2 selectors.
class IntMux4_1: public Component {
public:
	IntMux4_1(numeric inPut1, numeric inPut2, numeric inPut3, numeric inPut4, logicBit selector0, logicBit selector1, numeric outPut);
private:
	numeric val0Index, val1Index, val2Index, val3Index;
	logicBit sel0, sel1;
	void execute();
};

// AnalogIn: This function will read an analog channel and store the result in a numeric variable.
// The conversion is done as follows: numeric = analog * multiplier + offset
// Both multiplier and offset are floating point numbers, but the end result is stored in an unsigned integer.
// Take care when defining multiplier and offset - no sanity checks are made!
// NOTE!: The analog channel number must be 0...5. DO NOT USE ARDUINO A0...A5. THEY MAP INCORRECTLY HERE!
class AnalogIn: public Component {
public:
	AnalogIn(uint8_t analogChannel, numeric outPut, float offset, float multiplier);
private:
	int32_t offs;
	int32_t mul;
	void execute();
};

// CompareNumeric: Compares 2 numeric values and sets the output bit true or false depending on result
// The compare operation is defined as outPut = inPut1 <OPERATION> inPut2
// where <OPERATION> is one of:
// LT (less than), LE (less or equal), EQ (exactly equal), GE (greater or equal), GT (greater than)
class CompareNumeric: public Component {
public:
	CompareNumeric(numeric inPut1, numeric inPut2, logicBit outPut, compareOp cmp);
private:
	numeric inNum2;
	compareOp comp;
	void execute();
};

// ComponentList: Internal bookkeeping component to facilitate executing the ladder logic.
// One instance is created automatically (named CList).
// In Arduino setup() You MUST call CList.begin(); before creating any new Components
// In Arduino loop() you execute the ladder logic by including the instruction CList.execute();
// This will iterate through all declared components and execute each one once per loop()
class ComponentList {
public:
	void begin();
	bool add( Component *component );
	void execute();
private:
	uint8_t index;
	Component *list[MAXCOMPONENTS];
};

extern ComponentList CList;

#endif

