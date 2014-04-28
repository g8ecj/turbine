//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  pwm.c   -   This program interfaces with the PWM on an AVR CPU to implement a shunt load control
//
//  History:   1.0 - First release. 
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef _CONTROL_H
#define _CONTROL_H

#include <avr/eeprom.h>


#define LOADOFF   0
#define LOADON    1
#define LOADAUTO  2

#define MANUALOFF  3
#define MANUALON   4


extern int16_t gDump;
extern int16_t gLoad;
extern uint8_t charge_mode;
extern int16_t TargetC;

extern int16_t gVupper;
extern int16_t gVlower;
extern int16_t gInverter;
extern int16_t gAbsorbVolts;
extern int16_t gFloatVolts;
extern int16_t gBankSize;
extern int16_t gMinCharge;
extern int16_t gMaxCharge;
extern int16_t gDischarge;
extern int16_t gMaxDischarge;
extern int16_t gRPMMax;
extern int16_t gRPMSafe;


void control_init (void);
void run_control (void);
void do_command (char value);

#endif
