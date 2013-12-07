//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  rpm.c   -   This program interfaces with timer0 an AVR CPU to implement a techometer
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

// Include Files
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <cfg/macros.h>
#include <drv/timer.h>

#include "features.h"
#include "median.h"
#include "minmax.h"
#include "rtc.h"
#include "eeprommap.h"
#include "rpm.h"



uint16_t gPeriod;
int16_t gRPM, gMaxRPM;
int16_t gPoles;

MEDIAN RpmMedian;
MINMAX RpmHourMax;

void
rpm_init (void)
{

	// Enable timer3 overflow interrupt
	TIMSK3 = BV (TOIE3);


	TCCR3A = 0;                  // not using output compare pins
	TCCR3B = BV (CS32);          // Set Normal mode, CLK/256 prescaler

	DDRE &= ~BV (5);             // port E5 as input (INT5)
	PORTE |= BV (5);             // turn on pullup on E5
	EICRB |= BV (ISC50) | BV (ISC51);    // interrupt on rising edge
	EIMSK |= BV (INT5);          // enable int 5

	median_init (&RpmMedian, 10);
	minmax_init(&RpmHourMax, 60, true);

}

// eg. 500rpm = (500 / 60 * 6) Hz = 50Hz
// 50Hz = period 20ms

// 16MHz clock with 256 prescale = 16uS per count
// max period = 16uS * 65535 = 1.05S = 10rpm
// min period (assume < 1% error) = 16uS * 100 = 1.6mS = 625Hz = 6250rpm but limited to 1000
// cutin for 3m turbine ~= 150rpm (15Hz); expect count ~= 4200
void
run_rpm (void)
{
	uint16_t value;
	static uint32_t lastmin = 0;


	if (gPeriod)
	{
		// using the number of magnet pole pairs on the rotor
		// we count at a rate of 16MHz / 256 = 16uS per count
		// rpm = freq * 60 / numpoles
		// freq = 10e6/period(uS)
		// rpm = 1000000/period * 16 * 60 / numpoles

		value = (1000000L / 16) * (60 / gPoles) / gPeriod;      // note order to prevent overflow
		if (value < 1000)
			median_add (&RpmMedian, value);
		median_getAverage (&RpmMedian, &gRPM);

	}
	else
		gRPM = 0;

	// see if a minute has passed, if so advance the pointer to track the last hour
	if (time () >= lastmin + 60)
	{
		lastmin = time ();
		minmax_add(&RpmHourMax);
	}

	gMaxRPM = minmax_get(&RpmHourMax, gRPM);

}



ISR (INT5_vect)
{
	gPeriod = TCNT3;
	TCNT3 = 0;
}

// timer interrupt - when hit this means we overflowed to set max count value
ISR (TIMER3_OVF_vect)
{
	gPeriod = 0;

}
