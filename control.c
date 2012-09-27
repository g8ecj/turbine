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
#include <drv/ow_1wire.h>
#include <drv/ow_ds2438.h>
#include <drv/ow_ds2413.h>
#include <drv/ow_ds18x20.h>

#include "tlog.h"
#include "features.h"
#include "utils.h"
#include "rtc.h"
#include "measure.h"
#include "control.h"

extern Serial serial;


// globals
int16_t gVupper;
int16_t gVlower;
int16_t absorbVolts;
int16_t floatVolts;
int16_t gLoad;
int16_t gShunt;
int16_t gInverter;
int16_t bankSize;					  // was gTargetHI - actual bank size
int16_t minCharge;				  // was gTargetLO - minimum level to disharge to when in auto mode


int16_t EEMEM eeVupper;
int16_t EEMEM eeVlower;
int16_t EEMEM eeabsorbVolts;
int16_t EEMEM eefloatVolts;
int16_t EEMEM eeInverter;
int16_t EEMEM eebankSize;		  // was gTargetHI - actual bank size
int16_t EEMEM eeminCharge;		  // was gTargetLO - minimum level to disharge to when in auto mode


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
	DDRB |= BV(5);					  //output on OCP1A pin (portb.5)

	OCR1A = 0;						  // Set a initial value in the OCR1A-register

	if (gpioid >= 0)				  // see if a DS2413 chip is present
		ToggleState(ids[gpioid], false);	// ensure we start at a known state

	TargetC = bankSize;

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

	// decide what charging mode we are in.
	if (gCharge < (bankSize * 0.90))
	{
		charge_mode = BULK;
		// only run shunt if volts gets stupidly high!
		VoltsHI = gVupper;
		VoltsLO = gVupper * 0.99;
	}
	else if (gCharge < bankSize)
	{
		charge_mode = ABSORB;
		// start throttling back once over float volts, never go above absorb volts
		VoltsHI = absorbVolts;
		VoltsLO = floatVolts;
	}
	else
	{
		charge_mode = FLOAT;
		// never go above float volts
		VoltsHI = floatVolts;
		VoltsLO = floatVolts * 0.99;
	}

	// see if we are above shunt load threshold
	diff = gVolts - VoltsLO;
	if (diff > 0)
	{
		// see what range we're operating the PWM over
		range = VoltsHI - VoltsLO;
		// if above the top value then set near max on time but make sure its still pulsing
		// in case we are using AC coupling!!
		if (diff > (int16_t) range)
			regval = 1000;
		else
			regval = 1000 / range * diff;

		OCR1A = regval;
		tmp = gShunt;
		gShunt = regval / 10;	  // shunt load is activated - show initial value
		if (tmp == 0)				  // if going from OFF to ON then log the event
			log_event(LOG_SHUNTON);
	}
	else
	{
		OCR1A = 0;
		if (gShunt > 0)
		{
			gShunt = 0;
			log_event(LOG_SHUNTOFF);
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
			TargetC = bankSize;
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
			TargetC = bankSize;
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
				TargetC = minCharge;
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
				TargetC = bankSize;
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
