/* hardware config */


// code debugging!!
#define DEBUG 1

// debugging uses the serial port rather than a pushbutton array
#define PUSHBUTTONS 1

// defaults defined by the code used when <right> is pressed during field edit

#define ddVlower         2200         // volt limit low
#define ddVupper         2900         // volt limit high
#define ddFloatVolts     2700         // shunt volt low
#define ddAbsorbVolts    2800         // shunt volt high
#define ddBankSize       1000         // charge high
#define ddMinCharge       500         // charge low
#define ddMaxCharge       900         // charge low
#define ddCharge         1000         // sync
#define ddVoltage          24         // system voltage

#define ddHOUR             12         // hour
#define ddMINUTE            0         // minute
#define ddSECOND            0         // second
#define ddDAY              15         // day
#define ddMONTH             6         // month
#define ddYEAR             13         // year

#define ddInverter          1         // control active
#define ddLoad              0         // manual on/off

#define ddShunt          1000         // shunt conductance in Siemens
#define ddPoles             6         // magnetic poles in generator
#define ddSelfDischarge    10         // battery leakage in days for 1% loss
#define ddIdleCurrent       2         // idle current of controller, router etc
#define ddAdjustTime        0         // clock adjuster

#define ddUsdate            0         // default to Euro date format

