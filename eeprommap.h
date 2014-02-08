//---------------------------------------------------------------------------
// Copyright (C) 2013 Robin Gilks
//
//
//  eeprommap.h   -   All the variables that are in eeprom are declared here
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

// include files

#ifndef _EEPROMMAP_H_
#define _EEPROMMAP_H_

#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include "rtc.h"


// configurated max voltage
extern int16_t EEMEM eeVupper;
// configurated min voltage
extern int16_t EEMEM eeVlower;
// configurated absorb voltage
extern int16_t EEMEM eeAbsorbVolts;
// configurated float voltage
extern int16_t EEMEM eeFloatVolts;
// configurated inverter control
extern int16_t EEMEM eeInverter;
// configurated battery bank size
extern int16_t EEMEM eeBankSize;
// configurated min charge level to discharge to in auto mode
extern int16_t EEMEM eeMinCharge;
// configurated system voltage to nearest 6 volts
extern int16_t EEMEM eeVoltage;
// multiplier used to calibrate voltage readings
extern float EEMEM eeVoffset;
// configurated shunt resistor value in Siemens
extern int16_t EEMEM eeShunt;
// configurated number of poles in the generator (used for RPM measurement
extern int16_t EEMEM eePoles;
// configurated number of seconds per day to adjust clock for slow/fast 16MHz crystal
extern int16_t EEMEM eeAdjustTime;
// configurated value in amps of all the control circuitry (controller on its own is 3!)
extern int16_t EEMEM eeIdleCurrent;
// configurated day/month order for US or Euro display
extern int16_t EEMEM eeUSdate;
// configurated time in days for batteries to self discharge 1%
extern uint16_t EEMEM eeSelfDischarge;


// charge level updated every hour or every 10 amp hrs
extern int16_t EEMEM eeCharge;
// last time the self leakage was applied
extern uint32_t EEMEM eeSelfLeakTime;
// running total of idle current being summed until its large enough to affect DCS register
extern int16_t EEMEM eeIdleTotal;
// date and time stored when set and every hour so clock isn't too far out after a reset
extern DT_t EEMEM eeDateTime;

// configured max charge level to discharge to in auto mode
extern int16_t EEMEM eeMaxCharge;
// charge level updated every time we finish a discharge cycle
extern int16_t EEMEM eeDischarge;
// how many times to run discharge cycle before do float charge
extern int16_t EEMEM eeMaxDischarge;



void load_eeprom_values(void);
void save_eeprom_values(void);

#endif


