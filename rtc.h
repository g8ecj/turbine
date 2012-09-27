//*****************************************************************************
//
//  File........: RTC.h
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Description.: Functions for RTC.c
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - File created                                  - LHM
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//
//*****************************************************************************


extern int16_t gSECOND;
extern int16_t gMINUTE;
extern int16_t gHOUR;
extern int16_t gDAY;
extern int16_t gMONTH;
extern int16_t gYEAR;


//  Function declarations
void rtc_init (void);           //initialize the Timer Counter 2 in asynchron operation
void run_rtc (void);        //updates the time and date
// time in seconds since midnight, 1st Jan 2000
uint32_t time(void);
void set_epoch_time(void);
void get_datetime(uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* min, uint8_t* sec);

