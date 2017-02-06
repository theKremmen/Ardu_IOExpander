/*
 * plcconfig.h
 *
 * Created: 11.1.2017 9:34:45
 *  Author: Kremmen
 * This header configures the simple logic controller
 * For performance reasons no runtime checking is done so configure with care!
 */ 


#ifndef PLCCONFIG_H_
#define PLCCONFIG_H_

// TIMERTICK: The resolution of the timers in components that time something.
// The interval is given in microseconds, smaller is quicker.
// The default value 1000 is one tick every millisecond.
// All components share the same tick so select a value that is 
// small enough but no smaller. Too small interval will overload
// the processor
#define TIMERTICK 1000

// SPICLOCK: The clock rate of the SPI serial clock that transfers input and output bits
// Default is 1000000 i.e. 1 MHz 
#define SPICLOCK 1000000

// BITSPACE: memory space allocated for bit variables of the "ladder" logic
// One bit of memory is allocated for each bit variable.
// You can have bit variables numbered from 0 to (8 * BITSPACE)-1
// E.G. if BITSPACE = 32, then your bit index goes from 0 to 255.
// NOTE!: Bits 0...15 are the INPUTS, 16...31 are the OUTPUTS. The rest
// are freely available for programming.
// You may use OUTPUTS as inputs to logic elements, but you may NOT use
// INPUTS as outputs.
#define BITSPACE 32

#if BITSPACE <= 32
typedef uint8_t logicBit;
#else
typedef uint16_t logicBit;
#endif

// INTSPACE: memory space allocated for numeric variables related to timing, counting etc
// One uint16_t is allocated for each numeric. Be sure to check
// the number of variables a logic component reserves. As a rule, the bit operations
// do not reserve any numeric variables, neither do static timers or counters (where the timing or count is constant).
// Variable timers and counters need 1 or more. A multiplexer uses 3 or 5
#define INTSPACE 16
typedef uint8_t numeric;

// MAXTIMERS: The maximum number of timers reserved for the program
// Make sure this number is equal or larger than the actual number of timers
// used by all timing components together.
// As a general rule, any delay or pulse will use 1 timer.
#define MAXTIMERS 8

// MAXCOMPONENTS: The maximum number of components allowed in the program
// The list of components you create is maintained in a fixed size pointer array
// to avoid dynamic allocation and to minimize the list iteration runtime.
// Make sure this number is larger or equal to the actual number of all 
// ladder components in your application (anything you create using 'new').
#define MAXCOMPONENTS 64

// INVERT_INPUTS: Optionally you can invert the sense of all inputs by defining this (just remove the comment)
// All input '1's will turn to '0's and vice versa.
#define INVERT_INPUTS

#endif /* LOGICCONFIG_H_ */