//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  measure.c   -   This program measures the output from a wind turbine
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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include <algo/crc8.h>

#include <drv/timer.h>
#include <drv/ser.h>
#include <drv/ow_ds2438.h>
#include <drv/ow_ds2413.h>
#include <drv/ow_ds18x20.h>

#include "tlog.h"
#include <cfg/log.h>

#include "features.h"
#include "rtc.h"
#include "control.h"
#include "median.h"
#include "minmax.h"
#include "eeprommap.h"
#include "measure.h"

extern Serial serial;



#define NOMINALVOLTS 7			  /* try to keep input to the DS2438 about here */

// how long it takes to loose 1% of the total battery charge when idle
// in this case 7 days
#define SELFDISCHARGE 604800L

MEDIAN AmpsMedian;
MEDIAN VoltsMedian;
MEDIAN TempMedian;

MINMAX hourmax, daymax;
MINMAX hourmin, daymin;

int16_t gVolts;
int16_t iVolts;
int16_t gAmps;
int16_t gPower;
int16_t gShunt;
int16_t gTemp;
float gVoffset;
int16_t gVoltage;
uint16_t gDCA;
uint16_t gCCA;
int16_t gCharge;
int16_t gMaxhour, gMaxday;
int16_t gMinhour, gMinday;
int16_t gSelfDischarge;
int16_t gIdleCurrent;

CTX2438_t Result;

uint32_t self_discharge_time;

uint8_t ids[4][OW_ROMCODE_SIZE];	// only expect to find up to 3 actually!!
int8_t battid = -1, gpioid = -1, thermid = -1;





void
measure_init(void)
{
	uint8_t diff, cnt = 0;

	for (diff = OW_SEARCH_FIRST, cnt = 0; diff != OW_LAST_DEVICE; cnt++)
	{
		diff = ow_rom_search(diff, ids[cnt]);

		if ((diff == OW_PRESENCE_ERR) || (diff == OW_DATA_ERR))
			break;					  // <--- early exit!

#ifdef DEBUG
		kfile_printf(&serial.fd, "Found device %02x:%02x%02x%02x%02x%02x%02x:%02x\r\n", ids[cnt][0], ids[cnt][1],
						 ids[cnt][2], ids[cnt][3], ids[cnt][4], ids[cnt][5], ids[cnt][6], ids[cnt][7]);
		if (crc8(ids[cnt], 8))
			kfile_print(&serial.fd, "CRC suspect\r\n");
#endif
		if (ids[cnt][0] == SBATTERY_FAM)
			battid = cnt;
		if (ids[cnt][0] == SSWITCH_FAM)
			gpioid = cnt;
		if ((ids[cnt][0] == DS18S20_FAMILY_CODE) || (ids[cnt][0] == DS18B20_FAMILY_CODE) || (ids[cnt][0] == DS1822_FAMILY_CODE))
			thermid = cnt;

	}
	median_init(&AmpsMedian, 5);
	median_init(&VoltsMedian, 5);
	median_init(&TempMedian, 5);

	minmax_init(&hourmax, 60, true);
	minmax_init(&hourmin, 60, false);
	minmax_init(&daymax, 24, true);
	minmax_init(&daymin, 24, false);

	if (battid >= 0)                // see if a DS2438 chip is present
	{
		// load last saved charge value from eeprom
		eeprom_read_block((void *) &gCharge, (const void *) &eeCharge, sizeof(gCharge));
		// set mode and init expanded charge handler
		ow_ds2438_init(ids[battid], &Result, 1.0 / gShunt, gCharge);
	}
	// load last saved discharge time from eeprom
	eeprom_read_block((void *) &self_discharge_time, (const void *) &eeSelfLeakTime, sizeof(self_discharge_time));

}


int
do_calibration(void)
{

	// zero the current offset register
	return ow_ds2438_calibrate(ids[battid], &Result, 0);
}


int
do_CCADCA(int16_t percent, int16_t base)
{
	float t;

	if (percent == 0)
		t = 0.90;
	else
		t = (float)percent / 100.0;
	// if we haven't yet got anything recorded, then start at an arbitrary non-zero position
	if (Result.CCA < 10)
		Result.CCA = 10;
	// if we are passing in a value then use it
	if (base > 0)
		Result.CCA = base;

	// initialisation sets charge efficiency to 90%
	Result.DCA = (float)Result.CCA * t;
	return ow_ds2438_setCCADCA(ids[battid], &Result);
}


void
do_first_init(void)
{

	// initialise a totally fresh box by reseting the self-discharge timer
	self_discharge_time = time();
	eeprom_write_block((const void *) &self_discharge_time, (void *) &eeSelfLeakTime, sizeof(self_discharge_time));

}

// used to sync the value we have for battery charge state to the real battery (SG reading etc)
void
set_charge(uint16_t value)
{
	gCharge = value;
	ow_ds2438_init(ids[battid], &Result, 1.0 / gShunt, gCharge);
	eeprom_write_block((const void *) &gCharge, (void *) &eeCharge, sizeof(gCharge));
}


void
run_measure(void)
{
	float tmp;
	int16_t amps;
	static int8_t firstrun = true;
	static uint16_t lastcharge;
	static uint32_t lastmin = 0, lasthour = 0, lastday = 0;
	int16_t power;

	if (firstrun)
	{
		firstrun = false;

		lastmin = time();
		lasthour = time();
		lastday = time();
	}

	if (battid == -1)				  // see if a DS2438 chip is present
	{
		// dummy values if no hardware to read from
		gVolts = gVoltage * 105;
		gCharge = gBankSize * 0.90;
		return;
	}

	ow_ds2438_doconvert(ids[battid]);
	if (!ow_ds2438_readall(ids[battid], &Result))
		return;						  // bad read - exit fast!!

	// see if an external temperature sensor - if not then use what we have!!
	if (thermid == -1)
	{
		// smoothing function using a running average in a history buffer gets rid of glitches
		median_add(&TempMedian, Result.Temp);
		median_getAverage(&TempMedian, &gTemp);
	}

	// for current we get the median and the average values. Median removes glitches, average smooths bumps!
	median_add(&AmpsMedian, Result.Amps);
	median_getMedian(&AmpsMedian, &amps);
	median_getAverage(&AmpsMedian, &gAmps);

	// volts = as returned scaled by external divider; already scaled by 100, adjusted by calibration offset
	median_add(&VoltsMedian, (Result.Volts * gVoltage / NOMINALVOLTS) * gVoffset);
	// instantanious volts
	median_getMedian(&VoltsMedian, &iVolts);
	// smoothed (average) volts
	median_getAverage(&VoltsMedian, &gVolts);

	tmp = (float)gAmps * (float)gVolts;
	// volts & amps are scaled by 100 each so loose 10,000
	gPower = (int16_t) (tmp / 10000.0);

	// charge totals
	gCCA = Result.CCA;
	gDCA = Result.DCA;

	// remaining capacity (amp-hrs)
	gCharge = Result.Charge;

	// if the charge level has changed a lot the stash away in eeprom
	if (abs(lastcharge - gCharge) > 20)
	{
		lastcharge = gCharge;
		eeprom_write_block((const void *) &gCharge, (void *) &eeCharge, sizeof(gCharge));
	}

	/* keep max power for last hour and day */

	// the power goes directly into the current minute slot in an array of 60 slots (1 hour)
	// once a minute we update the pointer to this array so leaving the max for the last minute in the last slot

	// we scan this array of 60 minutes to find the highest value over the hour and copy that into the array of 24 hours (1 day)
	// we keep this maximum value over the hour for the UI to display
	// we scan the array of 24 hours and keep the maximum value for the UI to display
	// same goes for minimum values.

	tmp = (float)amps * (float)iVolts;
	// volts & amps are scaled by 100 each so loose 10,000
	power = (int16_t) (tmp / 10000.0);

	// log new power events while we have all the constituent values available
	if (power > gMaxhour)
		log_event(LOG_NEWHOURMAX);
	if (power > gMaxday)
		log_event(LOG_NEWDAYMAX);

	if (power < gMinhour)
		log_event(LOG_NEWHOURMIN);
	if (power < gMinday)
		log_event(LOG_NEWDAYMIN);

	// see if a minute has passed, if so advance the pointer to track the last hour
	if (time() >= lastmin + 60)
	{
		lastmin = time();
		minmax_add(&hourmax);
		minmax_add(&hourmin);

		// once per minute, try for an update from the external temperature sensor (if we have one)
		if (thermid >=0)
		{
			int16_t temperature;
			// select lowest resolution so its fast
			ow_ds18x20_resolution(ids[thermid], 9);
			ow_ds18X20_start (ids[thermid], false);
			while (ow_busy ());
			ow_ds18X20_read_temperature (ids[thermid], &temperature);
			median_add(&TempMedian, temperature);
			median_getAverage(&TempMedian, &gTemp);
		}
	}

	// save the present power level if greater than already there
	gMaxhour = minmax_get(&hourmax, power);
	gMinhour = minmax_get(&hourmin, power);

	// see if we have finished an hour, if so then move to a new hour
	if (time() >= lasthour + 3600)
	{
		lasthour = time();
		minmax_add(&daymax);
		minmax_add(&daymin);

		// once per hour save the charge level into eeprom
		lastcharge = gCharge;
		eeprom_write_block((const void *) &gCharge, (void *) &eeCharge, sizeof(gCharge));
	}

	gMaxday = minmax_get(&daymax, gMaxhour);
	gMinday = minmax_get(&daymin, gMinhour);

	// pessimistically assume 1% loss of battery charge per unit time - units in days
	if (time() >= (self_discharge_time + (uint32_t) ((float)gSelfDischarge * 3600.0 * 24.0)))
	{
		self_discharge_time = time();
		gCharge *= 0.99;
		ow_ds2438_init(ids[battid], &Result, 1.0 / gShunt, gCharge);
		eeprom_write_block((const void *) &self_discharge_time, (void *) &eeSelfLeakTime, sizeof(self_discharge_time));
		log_event(LOG_LEAKADJUST);
	}

	// see if we have finished a day, if so then total up the idle current
	if (time() >= lastday + 86400)
	{
		uint16_t total_idle;

		lastday = time();
		gCharge -= gIdleCurrent * 24 / 100;
		ow_ds2438_init(ids[battid], &Result, 1.0 / gShunt, gCharge);
		log_event(LOG_IDLEADJUST);
		// keep a running total of idle current until its big enough to influence DCA register
		eeprom_read_block((void *) &total_idle, (const void *) &eeIdleTotal, sizeof(total_idle));
		total_idle += gIdleCurrent * 24 / 100;
		if (total_idle > 64)
		{
			// indicate that more charge has gone from the battery
			Result.DCA += total_idle;
			ow_ds2438_setCCADCA(ids[battid], &Result);
			total_idle = 0;
		}
		eeprom_write_block((const void *) &total_idle, (void *) &eeIdleTotal, sizeof(total_idle));
	}

}

