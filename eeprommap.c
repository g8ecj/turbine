//---------------------------------------------------------------------------
// Copyright (C) 2013 Robin Gilks
//
//
//  eeprommap.c   -   All the variables that are in eeprom are declared here
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

#include "eeprommap.h"

#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include "control.h"
#include "measure.h"
#include "rpm.h"
#include "rtc.h"
#include "ui.h"

// these variables are all in one place so that if another is added, we don't 
// loose the existing value provided new stuff is *ALWAYS* added to the end


// configured max voltage
int16_t EEMEM eeVupper;
// configured min voltage
int16_t EEMEM eeVlower;
// configured absorb voltage
int16_t EEMEM eeAbsorbVolts;
// configured float voltage
int16_t EEMEM eeFloatVolts;
// configured inverter control
int16_t EEMEM eeInverter;
// configured battery bank size
int16_t EEMEM eeBankSize;
// configured min charge level to discharge to in auto mode
int16_t EEMEM eeMinCharge;
// configured system voltage to nearest 6 volts
int16_t EEMEM eeVoltage;
// multiplier used to calibrate voltage readings
float EEMEM eeVoffset;
// configured shunt resistor value in Siemens
int16_t EEMEM eeShunt;
// configured number of poles in the generator (used for RPM measurement
int16_t EEMEM eePoles;
// configured number of seconds per day to adjust clock for slow/fast 16MHz crystal
int16_t EEMEM eeAdjustTime;
// configured value in amps of all the control circuitry (controller on its own is 3!)
int16_t EEMEM eeIdleCurrent;
// configured day/month order for US or Euro display
int16_t EEMEM eeUSdate;
// configured time in days for batteries to self discharge 1%
uint16_t EEMEM eeSelfDischarge;


// charge level updated every hour or every 10 amp hrs
int16_t EEMEM eeCharge;
// last time the self leakage was applied
uint32_t EEMEM eeSelfLeakTime;
// running total of idle current being summed until its large enough to affect DCS register
int16_t EEMEM eeIdleTotal;
// date and time stored when set and every hour so clock isn't too far out after a reset
DT_t EEMEM eeDateTime;

// configured max charge level to discharge to in auto mode
int16_t EEMEM eeMaxCharge;
// charge level updated every time we finish a discharge cycle
int16_t EEMEM eeDischarge;
// how many times to run discharge cycle before do float charge
int16_t EEMEM eeMaxDischarge;
// max RPM before shutdown
int16_t EEMEM eeRPMMax;
// max RPM at which the big switch can be thrown
int16_t EEMEM eeRPMSafe;

void load_eeprom_values(void)
{

	eeprom_read_block ((void *) &gVupper, (const void *) &eeVupper, sizeof (gVupper));
	eeprom_read_block ((void *) &gVlower, (const void *) &eeVlower, sizeof (gVlower));
	eeprom_read_block ((void *) &gAbsorbVolts, (const void *) &eeAbsorbVolts, sizeof (gAbsorbVolts));
	eeprom_read_block ((void *) &gFloatVolts, (const void *) &eeFloatVolts, sizeof (gFloatVolts));
	eeprom_read_block ((void *) &gBankSize, (const void *) &eeBankSize, sizeof (gBankSize));
	eeprom_read_block ((void *) &gMinCharge, (const void *) &eeMinCharge, sizeof (gMinCharge));
	eeprom_read_block ((void *) &gMaxCharge, (const void *) &eeMaxCharge, sizeof (gMaxCharge));
	eeprom_read_block ((void *) &gMaxDischarge, (const void *) &eeMaxDischarge, sizeof (gMaxDischarge));
	eeprom_read_block ((void *) &gVoffset, (const void *) &eeVoffset, sizeof (gVoffset));
	eeprom_read_block ((void *) &gVoltage, (const void *) &eeVoltage, sizeof (gVoltage));
	eeprom_read_block ((void *) &gInverter, (const void *) &eeInverter, sizeof (gInverter));
	eeprom_read_block ((void *) &gRPMMax, (const void *) &eeRPMMax, sizeof (gRPMMax));
	eeprom_read_block ((void *) &gRPMSafe, (const void *) &eeRPMSafe, sizeof (gRPMSafe));
	eeprom_read_block ((void *) &gSelfDischarge, (const void *) &eeSelfDischarge, sizeof (gSelfDischarge));
	eeprom_read_block ((void *) &gIdleCurrent, (const void *) &eeIdleCurrent, sizeof (gIdleCurrent));
	eeprom_read_block ((void *) &gShunt, (const void *) &eeShunt, sizeof (gShunt));
	eeprom_read_block ((void *) &gPoles, (const void *) &eePoles, sizeof (gPoles));
	eeprom_read_block ((void *) &gUSdate, (const void *) &eeUSdate, sizeof (gUSdate));
	eeprom_read_block ((void *) &gAdjustTime, (const void *) &eeAdjustTime, sizeof (gAdjustTime));

}

void save_eeprom_values(void)
{
	eeprom_write_block ((const void *) &gVupper, (void *) &eeVupper, sizeof (gVupper));
	eeprom_write_block ((const void *) &gVlower, (void *) &eeVlower, sizeof (gVlower));
	eeprom_write_block ((const void *) &gAbsorbVolts, (void *) &eeAbsorbVolts, sizeof (gAbsorbVolts));
	eeprom_write_block ((const void *) &gFloatVolts, (void *) &eeFloatVolts, sizeof (gFloatVolts));
	eeprom_write_block ((const void *) &gBankSize, (void *) &eeBankSize, sizeof (gBankSize));
	eeprom_write_block ((const void *) &gMinCharge, (void *) &eeMinCharge, sizeof (gMinCharge));
	eeprom_write_block ((const void *) &gMaxCharge, (void *) &eeMaxCharge, sizeof (gMaxCharge));
	eeprom_write_block ((const void *) &gMaxDischarge, (void *) &eeMaxDischarge, sizeof (gMaxDischarge));
	eeprom_write_block ((const void *) &gVoffset, (void *) &eeVoffset, sizeof (gVoffset));
	eeprom_write_block ((const void *) &gVoltage, (void *) &eeVoltage, sizeof (gVoltage));
	eeprom_write_block ((const void *) &gInverter, (void *) &eeInverter, sizeof (gInverter));
	eeprom_write_block ((const void *) &gRPMMax, (void *) &eeRPMMax, sizeof (gRPMMax));
	eeprom_write_block ((const void *) &gRPMSafe, (void *) &eeRPMSafe, sizeof (gRPMSafe));
	eeprom_write_block ((const void *) &gSelfDischarge, (void *) &eeSelfDischarge, sizeof (gSelfDischarge));
	eeprom_write_block ((const void *) &gIdleCurrent, (void *) &eeIdleCurrent, sizeof (gIdleCurrent));
	eeprom_write_block ((const void *) &gShunt, (void *) &eeShunt, sizeof (gShunt));
	eeprom_write_block ((const void *) &gPoles, (void *) &eePoles, sizeof (gPoles));
	eeprom_write_block ((const void *) &gUSdate, (void *) &eeUSdate, sizeof (gUSdate));
	eeprom_write_block ((const void *) &gAdjustTime, (void *) &eeAdjustTime, sizeof (gAdjustTime));

}