/* hardware config */

// the value of the current shunt in ohms
#define Rshunt 0.001

// how many magnet pairs (i.e. poles) on the generator
#define ROTORMAGNETPAIRS   6

// optionally the battery configuration - bank capacity and voltage. Only used for defaults
#define BATTERYCAPACITY 450     /* amp-hours */
#define BATTERYVOLTAGE 24       /* volts */

// code debugging!!
#define DEBUG 1


#define LOG_LEVEL LOG_LVL_INFO

// debugging uses the serial port rather than a pushbutton array
#define PUSHBUTTONS 1
