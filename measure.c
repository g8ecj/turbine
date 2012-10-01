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
#include "features.h"
#include "utils.h"
#include "rtc.h"
#include "control.h"
#include "measure.h"

extern Serial serial;



#define NOMINALVOLTS 7			  /* try to keep input to the DS2438 about here */

// how long it takes to loose 1% of the total battery charge when idle
// in this case 7 days
#define SELFDISCHARGE 604800L

S_ARRAY ahistory;
int16_t ahbuff[10];
S_ARRAY vhistory;
int16_t vhbuff[10];
S_ARRAY thistory;
int16_t thbuff[30];


int16_t gVolts;
int16_t gAmps;
int16_t gPower;
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



uint8_t ids[3][OW_ROMCODE_SIZE];	// only expect to find 2 actually!!
int8_t battid = -1, gpioid = -1;


int16_t EEMEM eecharge;
int16_t EEMEM eeVoltage;
float EEMEM eeVoffset;
uint32_t EEMEM eeSelfLeakTime;
uint16_t EEMEM eeSelfDischarge;
int16_t EEMEM eeIdleCurrent;


char ToggleState(uint8_t * id, uint8_t state);





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
	}

	smooth_init(&vhistory, vhbuff, BUFSIZE(vhbuff));
	smooth_init(&ahistory, ahbuff, BUFSIZE(ahbuff));
	smooth_init(&thistory, thbuff, BUFSIZE(thbuff));

	if (battid >= 0)                // see if a DS2438 chip is present
	{
		// load last saved charge value from eeprom
		eeprom_read_block((void *) &gCharge, (const void *) &eecharge, sizeof(gCharge));
		// set mode and init expanded charge handler
		ow_ds2438_init(ids[battid], &Result, Rshunt, gCharge);
	}

}


int
do_calibration(void)
{
	return ow_ds2438_calibrate(ids[battid], &Result, gIdleCurrent);	// offset 0 = calculate it
}

int
do_ClearCCADCA(void)
{
	Result.CCA = 10;
	Result.DCA = 9;
	return ow_ds2438_setCCADCA(ids[battid], &Result);	// set both CCA and DCA
}

// used to sync the value we have for battery charge state to the real battery (SG reading etc)
void
set_charge(uint16_t value)
{
	gCharge = value;
	ow_ds2438_init(ids[battid], &Result, Rshunt, gCharge);
	eeprom_write_block((const void *) &gCharge, (void *) &eecharge, sizeof(gCharge));
}


void
run_measure(void)
{
	float volts, amps, tmp;
	static int8_t firstrun = true;
	static uint16_t lastcharge;
	static int16_t hourmax[60], daymax[24];
	static int16_t hourmin[60], daymin[24];
	static uint32_t lastmin = 0, lasthour = 0, self_discharge_time;
	static uint8_t minptr = 0, hourptr = 0;
	int16_t power, j;

	if (battid == -1)				  // see if a DS2438 chip is present
	{
		gVolts = gVoltage * 105;
		gCharge = bankSize * 0.90;
		return;
	}

	ow_ds2438_doconvert(ids[battid]);
	if (!ow_ds2438_readall(ids[battid], &Result))
		return;						  // bad read - exit fast!!

	// degrees = value * 0.03125:: scaled up by 100
	gTemp = smooth(&thistory, Result.Temp);
	// amps = register value / (4096 * shunt); scaled by 100
	amps = Result.Amps;
	// volts = as returned scaled by external divider; already scaled by 100, adjusted by calibration offset
	volts = (Result.Volts * gVoltage / NOMINALVOLTS) * gVoffset;

	if (firstrun)
	{
		firstrun = false;

		for (j = 0; j < 60; j++)
		{
			hourmax[j] = -9999;
			hourmin[j] = 9999;
		}
		for (j = 0; j < 24; j++)
		{
			daymax[j] = -9999;
			daymin[j] = 9999;
		}
		gMaxhour = -9999;
		gMaxday = -9999;
		gMinhour = 9999;
		gMinday = 9999;

		// load last saved discharge time from eeprom
		eeprom_read_block((void *) &self_discharge_time, (const void *) &eeSelfLeakTime, sizeof(self_discharge_time));
		if ((self_discharge_time == 0) || (self_discharge_time == 0xffffffff))
		{
			// initialise a totally fresh box
			self_discharge_time = time();
			eeprom_write_block((const void *) &self_discharge_time, (void *) &eeSelfLeakTime, sizeof(self_discharge_time));
		}
	}

	gCCA = Result.CCA;
	gDCA = Result.DCA;

	// remaining capacity (amp-hrs)
	gCharge = Result.Charge;

	// if the charge level has changed a lot the stash away in eeprom
	if (abs(lastcharge - gCharge) > 20)
	{
		lastcharge = gCharge;
		eeprom_write_block((const void *) &gCharge, (void *) &eecharge, sizeof(gCharge));
	}

	// smoothing function using a running average in a history buffer
	gVolts = smooth(&vhistory, volts);
	gAmps = smooth(&ahistory, amps);

	tmp = (float) gAmps *(float) gVolts;
	// volts & amps are scaled by 100 each so loose 10,000
	gPower = (int16_t) (tmp / 10000.0);

	/* keep max power for last hour and day */

	// the power goes directly into the current minute slot in an array of 60 slots (1 hour)
	// once a minute we update the pointer to this array so leaving the max for the last minute in the last slot

	// we scan this array of 60 minutes to find the highest value over the hour and copy that into the array of 24 hours (1 day)
	// we keep this maximum value over the hour for the UI to display
	// we scan the array of 24 hours and keep the maximum value for the UI to display
	// same goes for minimum values.

	tmp = (float) amps *(float) volts;
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

	// pessimistically assume 1% loss of battery charge per week due to self disharge
	if (time() >= (self_discharge_time + (gSelfDischarge * 3600 * 24)))
	{
		gCharge *= 0.99;
		ow_ds2438_init(ids[battid], &Result, Rshunt, gCharge);
		self_discharge_time = time();
		eeprom_write_block((const void *) &self_discharge_time, (void *) &eeSelfLeakTime, sizeof(self_discharge_time));
		log_event(LOG_LEAKADJUST);
	}

	// see if a minute has passed, if so advance the pointer to track the last hour
	if (time() >= lastmin + 60)
	{
		lastmin = time();
		minptr++;
		if (minptr >= 60)
			minptr = 0;				  // reset to the start of the hour array
		hourmax[minptr] = -9999;		  // clear the next slot in the array
		hourmin[minptr] = 9999;		    // clear the next slot in the array
	}

	// save the present power level if greater than already there
	if (power > hourmax[minptr])
		hourmax[minptr] = power;

	// save the present power level if less than already there
	if (power < hourmin[minptr])
		hourmin[minptr] = power;

	// find a new maximum for this last hour & save for the UI and day list
	gMaxhour = -9999;
	for (j = 0; j < 60; j++)
		if (hourmax[j] > gMaxhour)
			gMaxhour = hourmax[j];

	// find a new minimum for this last hour & save for the UI and day list
	gMinhour = 9999;
	for (j = 0; j < 60; j++)
		if (hourmin[j] < gMinhour)
			gMinhour = hourmin[j];

	// see if we have finished an hour, if so then move to a new hour
	if (time() >= lasthour + 3600)
	{
		lasthour = time();
		hourptr++;
		if (hourptr >= 24)
			hourptr = 0;
		daymax[hourptr] = -9999;
		daymin[hourptr] = 9999;

		lastcharge = gCharge;
		eeprom_write_block((const void *) &gCharge, (void *) &eecharge, sizeof(gCharge));
	}

	// put max in the last hour into the day list if its bigger
	if (daymax[hourptr] < gMaxhour)
		daymax[hourptr] = gMaxhour;

	// put min in the last hour into the day list if its smaller
	if (daymin[hourptr] > gMinhour)
		daymin[hourptr] = gMinhour;

	/* find the new max in the last day for the UI */
	gMaxday = -9999;
	for (j = 0; j < 24; j++)
		if (daymax[j] > gMaxday)
			gMaxday = daymax[j];

	/* find the new min in the last day for the UI */
	gMinday = 9999;
	for (j = 0; j < 24; j++)
		if (daymin[j] < gMinday)
			gMinday = daymin[j];
}

