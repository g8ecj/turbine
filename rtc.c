//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  rtc.c   -   Real time clock - provides date, time and seconds since 1st Jan 1970
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

#include <drv/timer.h>
#include <avr/eeprom.h>

#include "rtc.h"


int16_t gSECOND;
int16_t gMINUTE;
int16_t gHOUR;
int16_t gDAY;
int16_t gMONTH;
int16_t gYEAR;
static volatile uint32_t Epoch;
static volatile ticks_t LastTicks;

typedef struct datetime
{
   uint16_t d;
   uint16_t m;
   uint16_t y;
   uint16_t S;
   uint16_t M;
   uint16_t H;
} DT_t;

DT_t EEMEM eeDateTime;

#define HOUR       0
#define MINUTE     1
#define SECOND     2
#define NOFLASH    3

#define YEAR        0
#define MONTH       1
#define DAY         2


// Lookup table holding the length of each mont. The first element is a dummy.
//    this could be placed in progmem too, but the arrays are accessed quite
//    often - so leaving them in RAM is better...
char MonthLength[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


// time in seconds since midnight, 1st Jan 1970
void
set_epoch_time (void)
{
	uint8_t i;
	uint32_t t;
	DT_t DateTime;

	t = (gYEAR * 365);			  // days in year
	for (i = 0; i < gYEAR; i += 4)
		t += 1;						  // leap years (includes 2000, excludes current year)

	if ((gMONTH > 2) && ((gYEAR & 3) == 0))
		t += 1;						  // add leap day for this leap year

	for (i = 0; i < gMONTH; i++)
		t += MonthLength[i];		  // days in month (not including this month!!)

	t += gDAY - 1;					  // don't include today!!
	t *= 24;							  // days -> hours
	t += gHOUR;
	t *= 60;
	t += gMINUTE;
	t *= 60;
	t += gSECOND;
	t += 946638000;				  // correction to base to 1970 (Unix time)

	Epoch = t;

	DateTime.S = gSECOND;
	DateTime.M = gMINUTE;
	DateTime.H = gHOUR;
	DateTime.d = gDAY;
	DateTime.m = gMONTH;
	DateTime.y = gYEAR;
	eeprom_write_block ((const void *) &DateTime, (void *) &eeDateTime, sizeof (DateTime));

}

void get_datetime(uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	*sec = gSECOND;
	*min = gMINUTE;
	*hour = gHOUR;
	*day = gDAY;
	*month = gMONTH;
	*year = gYEAR + 2000;

}


/******************************************************************************
*
*   Function name:  RTC_init
*
*   returns:        none
*
*   parameters:     none
*
*   Purpose:        Start Timer/Counter2 in asynchronous operation using a
*                   32.768kHz crystal.
*
*******************************************************************************/
void
rtc_init (void)
{
	DT_t DateTime;

	// initial time and date setting
	eeprom_read_block ((void *) &DateTime, (const void *) &eeDateTime, sizeof (DateTime));
	gSECOND = DateTime.S;
	gMINUTE = DateTime.M;
	gHOUR = DateTime.H;
	gDAY = DateTime.d;
	gMONTH = DateTime.m;
	gYEAR = DateTime.y;

	LastTicks = timer_clock ();
	set_epoch_time ();
}


uint32_t
time (void)
{
	return Epoch;
}






/******************************************************************************
*
*   Timer/Counter2 Overflow Interrupt Routine
*
*   Purpose: Increment the real-time clock
*            The interrupt occurs once a second (running from the 32kHz crystal)
*
*******************************************************************************/

void
run_rtc (void)
{
	int8_t LeapMonth;
	int32_t diff;

	// find out how far off the exact number of ticks we are
	diff = timer_clock () - LastTicks - ms_to_ticks (1000);
	if (diff < 0)
		return;

	// add in the number of ticks we drifted by
	LastTicks = timer_clock () - diff;

	Epoch++;              // count seconds since epoch (1st Jan 1970)

	gSECOND++;            // increment second

	if (gSECOND == 60)
	{
		gSECOND = 0;
		gMINUTE++;

		if (gMINUTE > 59)
		{
			gMINUTE = 0;
			gHOUR++;
			set_epoch_time ();           // used to save time to eeprom

			if (gHOUR > 23)
			{
				gHOUR = 0;
				gDAY++;

				// Check for leap year if month == February
				if (gMONTH == 2)
					if (!(gYEAR & 0x0003))     // if (gYEAR%4 == 0)
						LeapMonth = 1;
					else
						LeapMonth = 0;
				else
					LeapMonth = 0;

				// Now, we can check for month length
				if (gDAY > (MonthLength[gMONTH] + LeapMonth))
				{
					gDAY = 1;
					gMONTH++;

					if (gMONTH > 12)
					{
						gMONTH = 1;
						gYEAR++;
					}
				}
			}
		}
	}
}
