//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  control.c   -   This program controls the load on a wind turbine
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
#include <math.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include <algo/crc8.h>

#include <drv/timer.h>
#include <drv/ser.h>
#include <drv/ow_1wire.h>
#include <drv/ow_ds2438.h>
#include <drv/ow_ds2413.h>
#include <drv/ow_ds18x20.h>

#include "tlog.h"
#include "features.h"
#include "rtc.h"
#include "measure.h"
#include "eeprommap.h"
#include "control.h"

extern Serial serial;


// globals
int16_t gVupper;
int16_t gVlower;
int16_t gAbsorbVolts;
int16_t gFloatVolts;
int16_t gLoad;
int16_t gDump;
int16_t gInverter;
int16_t gBankSize;
int16_t gMinCharge;
int16_t gMaxCharge;
int16_t gDischarge;
int16_t gMaxDischarge;


int16_t TargetC;
uint8_t command = 0;
uint8_t charge_mode;


enum CHARGE
{
	BULK = 'B',
	ABSORB = 'A',
	FLOAT = 'F'
};


char ToggleState(uint8_t * id, uint8_t state);



#define PRESCALE ((1 << CS11))

void
control_init(void)
{
	//set Timer1 in fast pwm mode (7), TOP=0x3ff (1023)
	//clear output on compare match, set at top (COM1A1=1)
	TCCR1A = (1 << COM1A1) | (1 << WGM11) | (1 << WGM10);
	TCCR1B = (1 << WGM12) | PRESCALE;
	//output on OCP1A pin (portb.5)
	DDRB |= BV(5);

	// Set a initial value in the OCR1A-register
	OCR1A = 0;

	// see if a DS2413 chip is present so we can ensure we start at a known state
	if (gpioid >= 0)
		ToggleState(ids[gpioid], false);

	eeprom_read_block ((void *) &gDischarge, (const void *) &eeDischarge, sizeof(gDischarge));
	if (gDischarge == 0)
		TargetC = gBankSize;
	else
		TargetC = gMaxCharge;

}




// press the start/stop button on the inverter remote control and wait for the indicator
// LED to go ON or OFF to show success (done by relay and opto coupler)
char
ToggleState(uint8_t * id, uint8_t state)
{

	uint8_t expected, read;

	if (gpioid == -1)				  // see if a DS2413 chip is present
		return true;				  // fake success

	read = ow_ds2413_read(id);

	if (state)
		expected = 0x4b;			  // state after toggling
	else
		expected = 0x0f;

	// if already in the correct state then no need to 'press' the switch
	if (read == expected)
		return true;

	if (!ow_ds2413_write(id, 2)) // set PIOA
		return false;
	timer_delay(500);
	if (!ow_ds2413_write(id, 3)) // clear PIOA
		return false;

	// allow time for the LED to come on or go off
	timer_delay(150);
	read = ow_ds2413_read(id);	  // read again
	if (read == expected)
		return true;
	else
		return false;
}




// control a load that is used when the battery bank is full
// Also used for last resort overvolt protection
void
run_control(void)
{
	// locals
	int16_t VoltsHI = 0;
	int16_t VoltsLO = 0;
	int16_t diff;
	uint16_t regval, range = 0, tmp;
	static bool log_reported = false;

	// decide what charging mode we are in.
	if (gCharge < (gBankSize * 0.90))
	{
		charge_mode = BULK;
		// only run shunt if volts gets stupidly high!
		VoltsHI = gVupper;
		VoltsLO = gVupper * 0.98;
	}
	else if (gCharge < gBankSize)
	{
		charge_mode = ABSORB;
		// start throttling back once over float volts, never go above absorb volts
		VoltsHI = gAbsorbVolts;
		VoltsLO = gFloatVolts;
	}
	else
	{
		charge_mode = FLOAT;
		// never go above float volts, but allow a bit of slack
		VoltsHI = gFloatVolts;
		VoltsLO = gFloatVolts * 0.98;
	}

	// compensate for temperature - assume set values are for 25C, adjust accordingly
	// the higher the temp, the lower the volts by 5mV per deg C
	// Tricky calculation as both volts and temperature are scaled by 100
	VoltsHI -= 0.005 * (gTemp - 2500);
	VoltsLO -= 0.005 * (gTemp - 2500);

	// see if we are above shunt load threshold
	diff = gVolts - VoltsLO;
	if (diff > 0)
	{
		// see what range we're operating the PWM over
		range = VoltsHI - VoltsLO;
// decide linear or log application of dump load
#if 0
		regval = 1010 / range * diff;
#else
#define SHAPE 63
		float dval = (float)diff / range;
		float fig = log(SHAPE);
		regval = 1010 * (exp(fig * dval) - 1) / (SHAPE - 1);
#endif
		// if above the top value then set near max on time but make sure its still pulsing
		// in case we are using AC coupling!!
		if (regval > 1010)
			regval = 1010;

		OCR1A = regval;
		tmp = gDump;
		gDump = regval / 10;	  // shunt load is activated - show initial value
		if (!log_reported && gDump >= 50)				  // if going from OFF to ON then log the event
		{
			log_event(LOG_SHUNTON);
			log_reported = true;
		}
	}
	else
	{
		OCR1A = 0;
		gDump = 0;
		if (log_reported)
		{
			log_event(LOG_SHUNTOFF);
			log_reported = false;
		}
	}

	// see if we have an inverter we can control
	if (gInverter == 0)
		return;


	if (command == MANUALOFF)
	{
		command = 0;
		if (ToggleState(ids[gpioid], false))
		{
			// turned off load OK, start charging
			gLoad = LOADOFF;
			log_event(LOG_MANUALOFF);
			TargetC = gBankSize;
		}
		else
		{
			// error
			log_event(LOG_MANUALOFF | LOG_ERROR);
		}
	}

	// we assume that the user knows what they are doing here!!
	// Manual override will run the inverter until the battery hits minimum voltage or manually switched off
	if (command == MANUALON)
	{
		command = 0;
		if (ToggleState(ids[gpioid], true))
		{
			gLoad = LOADON;
			log_event(LOG_MANUALON);
			// define point to which we discharge to
			TargetC = 0;
		}
		else
		{
			// error
			log_event(LOG_MANUALON | LOG_ERROR);
		}
	}


	// if volts are below set minimum then cut the load whatever the charge state is and start charging again
	// this ensures a retry at shutoff in case we get a failure (sticky relay etc)
	if (gVolts < gVlower)
	{
		if (ToggleState(ids[gpioid], false))
		{
			log_event(LOG_UNDERVOLT);
			// turned off load OK, start charging
			gLoad = LOADOFF;
			TargetC = gBankSize;
		}
		else
		{
			//error
			log_event(LOG_UNDERVOLT | LOG_ERROR);
		}
	}

	// see if we have an inverter we allow automatic control of
	if (gInverter != 2)
		return;


	// the load is not on
	if (gLoad == LOADOFF)
	{
		// if volts is above the set maximum apply load to try and pull the volts down - leave load on for only a short while
		if (gVolts > gVupper)
		{
			if (ToggleState(ids[gpioid], true))
			{
				// turn on load
				gLoad = LOADON;
				log_event(LOG_OVERVOLT);
				// set the target level to discharge to a small amount below the current value so we don't keep the load on for too long
				TargetC = gCharge * 0.99;
			}
			else
			{
				//error
				log_event(LOG_OVERVOLT | LOG_ERROR);
			}
		}
		// if we have reached our target charge level then start normal discharge cycle
		else if (gCharge >= TargetC)
		{
			if (ToggleState(ids[gpioid], true))
			{
				// turn on load
				gLoad = LOADAUTO;
				log_event(LOG_CHARGED);
				TargetC = gMinCharge;
			}
			else
			{
				// error
				log_event(LOG_CHARGED | LOG_ERROR);
			}
		}
	}
	// the load is already on or auto
	else
	{
		// if we have reached our target discharge level then start normal charge cycle
		if (gCharge <= TargetC)
		{
			if (ToggleState(ids[gpioid], false))
			{
				// turned off load OK, start charging
				gLoad = LOADOFF;
				log_event(LOG_DISCHARGED);
				// decide whether we are doing a normal charge or we are taking up to full float level
				if ((gDischarge > gMaxDischarge) || (gDischarge < 0))
				{
					gDischarge = 0;
					TargetC = gBankSize;
				}
				else
				{
					TargetC = gMaxCharge;
					gDischarge++;
				}
				eeprom_write_block ((const void *) &gDischarge, (void *) &eeDischarge, sizeof(gDischarge));
			}
			else
			{
				// error
				log_event(LOG_DISCHARGED | LOG_ERROR);
			}
		}
	}



}

// manual operation of the inverter through the UI
void
do_command(char value)
{
	command = value;
}
