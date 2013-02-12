//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  control.c   -   This program measures the output from and controls the load on a wind turbine
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

#ifndef _MEASURE_H
#define _MEASURE_H

#include <avr/eeprom.h>
#include <drv/ow_1wire.h>

extern int16_t gVolts;
extern int16_t gAmps;
extern int16_t gPower;
extern int16_t gShunt;

extern int16_t gTemp;
extern uint16_t gDCA;
extern uint16_t gCCA;
extern int16_t gCharge;
extern int16_t gMaxhour, gMaxday;
extern int16_t gMinhour, gMinday;
extern uint8_t ids[][OW_ROMCODE_SIZE];
extern int8_t battid, gpioid;
extern int16_t gSelfDischarge;
extern int16_t gIdleCurrent;

extern float gVoffset;
extern int16_t gVoltage;


void measure_init (void);
void run_measure (void);
char do_dump (char input);
char do_sync (char input);
void set_charge (uint16_t value);
int do_calibration (void);
int do_CCADCA(int16_t percent, int16_t base);

#endif
