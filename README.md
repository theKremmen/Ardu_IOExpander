Ardu_IOExpander README.
# A Really Poor Man's Programmable Logic Controller
The library plc.h/plc.cpp together with the associated IOExpander board implement a small open source/hw logic simulator. It is free for any use but comes with no warranties of any kind.
The functionality is open ended such that it is easy to implement additional logic blocks or port the whole gizmo to a more powerful platform (Arduino Due, ST Nucleo, maybe?).

## Configuring the PLC

Configure the PLC by editing the settings in "plcconfig.h" as follows:

**TIMERTICK:** ( default `#define TIMERTICK 1000 `).

The resolution of the timers in components that time something.
The interval is given in microseconds, thus smaller is quicker.
The default value 1000 is one tick every millisecond.
All components share the same tick so select a value that is small enough but no smaller. Too small interval will overload the processor

**SPICLOCK:** ( default `#define SPICLOCK 1000000` )

The clock rate of the SPI serial clock that transfers input and output bits. Default is 1000000 i.e. 1 MHz. There should be no need to adjust this but you may if there is a need.


**BITSPACE:** ( default `#define BITSPACE 32` )

Memory space allocated for bit variables of the "ladder" logic One bit of memory is allocated for each bit variable. You can have bit variables numbered from 0 to (8 * BITSPACE)-1 E.G. if BITSPACE = 32, then your bit index goes from 0 to 255.

NOTE!: The software assigns the physical input signals to bits 0...15 and bits 16...31 to the physical OUTPUTS. The rest are freely available for programming.
You may use OUTPUTS as inputs to logic elements, but you may **NOT** use INPUTS ( bits 0 ... 15) as outputs. (You can, but it won't work).


    #if BITSPACE <= 32
    typedef uint8_t logicBit;	// type for bit variable references
    #else
    typedef uint16_t logicBit;
    #endif
    
Also note that if you specify a bitspace > 32 bytes in size, the size of the bit reference changes fron uint8_t to uint16_t. This has not been tested so surprises are possible.

**INTSPACE:** ( default `#define INTSPACE 16` )

Memory space allocated for numeric variables related to timing, counting etc.

One uint16_t is allocated for each numeric. Be sure to check the number of variables a logic component reserves. As a rule, the bit operations do not reserve any numeric variables, neither do static timers or counters (where the timing or count is constant).

Variable timers and counters need 1 or more. A multiplexer uses 3 or 5

    typedef uint8_t numeric;	// type for numeric variable references

**MAXTIMERS:** ( default `#define MAXTIMERS 8` )

The maximum number of timers reserved for the program
Make sure this number is equal or larger than the actual number of timers used by all timing components together.
As a general rule, any delay or pulse will use 1 timer.


**MAXCOMPONENTS:** (default `#define MAXCOMPONENTS 64` )

The maximum number of components allowed in the program. 
The list of components you create is maintained in a fixed size pointer array to avoid dynamic allocation and to minimize the list iteration runtime.

Make sure this number is larger or equal to the actual number of all ladder components in your application (anything you create using 'new').


**INVERT_INPUTS:** ( default `#define INVERT_INPUTS` )

Optionally you can invert the sense of all inputs by defining this (the default is `#define`d )
All input '1's will turn to '0's and vice versa.

The IOExpander physical inputs are active low with internal pullup. Unconnected, the input will be pulled to +5V, i.e. '1' or 'high' and the LED will be dark. That is what the program sees if there is nothing connected to the input. The input LED illuminates when the input signal wire is grounded. This causes the program to read a '0' from the input. It works but reversing the input in the program makes it easier for the programmer. Also, the logic modules are clocked on signal rising edges.

So, when the inputs are inverted, grounding an input pin will cause the program to see a logic '1' in the corresponding input signal.


## Arduino

You need to install an ***original Arduino Micro*** or an ***exact clone***. Only those will have the SPI signals in the module pins. This feature is not configurable, so take care. There are lots of various "Arduino Micro Pro" modules and similar with different pinout in eBay and elsewhere - **those will not work!** Specifically, you cannot use an Arduino Nano as it does not have the necessary SPI signals in the pinout.

Install the Arduino so that the USB connector faces the board edge. You can solder the Arduino directly on the board or use a set of female headers if you wish to remove the module at some time. Programming can be done with everything in place, though.


## Power and signal wiring

1. Connect the supply ( +12VDC ) to the 2 position connector block in the lower middle of the board (when oriented so that the Ardu USB connector is facing up). Positive wire is the left one, ground the right. The supply is protected against reversed polarity.


2. Connect the sensors to the 3 pin headers on the left. The pins are marked on the silkscreen thus: [+ s -] where +: +5V supply to an active sensor; s: input signal ( 0...+5V ) to the PLC; -: ground. The input signal activated upon grounding the signal pin. Connecting the signal pin to +5V does nothing, but take care not to short the +5V and ground pins!

3. Connect the outputs to the right hand side 2 pin connectors. If the polarity matters, the positive pin is on the right when looking towards the board from outside. Take care not to short the positive pins to ground as there is no overcurrent protection on the expander board! (one hopes there is a current limit in the +12V supply you use).

## Bonus I/O

Most of the regular Arduino Micro digital and analog I/O signals have been brought out next to the Ardu module into normal through hole pads.

Digital signals D4 ... D10 can be found on the right side in a .1" pin header with a ground pin topmost. Use as you please with an extra header or solder the wires directly on board.

Similarly, analog signals A0 ... A5 can be found on the left side in a 3 pin header each. Pin 1 (rightmost, square pad) of each header is +5V for a pot or sensor or whatever, and the leftmost pin (3) is ground. The analog input is from pin 2 and there is a small RC lowpass connected to each input. Again, to avoid magic smoke do not short the +5V pins.

Be aware that the Arduino analog signals are pretty noisy and the base electronics don't really address that issue. So you need to take that into account when designing a PLC app that uses the analog values for something.

A solder pad to the left of the analog inputs, marked as 'Vref', is connected (surprise) to the analog reference pin of the MCU. Use as you wish or leave open. Also, close by is a pad marked 'D201' that connects to I/O pin D13 and the LED on board the Arduino. Now you can easily replicate the LED! Finally, on the other side of the Ardu there is a pad 'SS' that connects to USART_RXLED/SS. If you find a use for that, go right ahead.
 
Additinally, the board includes placeholders for I2C connectors and and SPI expansion to enable connecting units in series. These have not been tested but there should be no reason why they wouldn't work. If you need them, presumably you know what to do. Look to the right side of the Ardu module and you will find 2 solder bridges marked 'I2C TERM'. Shorting those will activate the 4.7k pull-up termination for the I2C bus.
