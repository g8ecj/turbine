//---------------------------------------------------------------------------
// Copyright (C) 2014 Robin Gilks
//
//
//  graph.c   -   This program measures the output from a wind turbine
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

#include <drv/timer.h>
#include <drv/ser.h>
#include <drv/ow_ds2438.h>
#include <drv/ow_ds2413.h>
#include <drv/ow_ds18x20.h>

#include "tlog.h"
#include <cfg/log.h>

#include "features.h"
#include "median.h"
#include "graph.h"
#include "rtc.h"
#include "measure.h"


MEDIAN PowerMins;
MEDIAN PowerHours;
MEDIAN PowerDays;



void
graph_init (void)
{
	median_init(&PowerMins, 60);
	median_init(&PowerHours, 60);
	median_init(&PowerDays, 24);
}

void
run_graph (void)
{
	static uint32_t lastsec = 0, lastmin = 0, lasthour = 0, lastday = 0;
	static int32_t pLastMin = 0, pLastHour = 0, pLastDay = 0;


	// see if a second has passed, if so add power into minute accumulator
	if (uptime() >= lastsec + 1)
	{
		pLastMin += gPower;
		lastsec = uptime();
	}

	// see if a minute has passed, if so advance the pointer to track the last hour
	if (uptime() >= lastmin + 60)
	{
		median_add(&PowerMins, (int16_t)pLastMin / 60);
		pLastMin = 0;
		lastmin = uptime();
	}

	// see if a minute has passed, if so advance the pointer to track the last hour
	if (uptime() >= lasthour + 3600)
	{
		int16_t tmp;
		median_getbyindex(&PowerMins, 1, &tmp);
		pLastHour += tmp;
		median_add(&PowerHours, (int16_t)pLastHour / 60);
		pLastHour = 0;
		lasthour = uptime();
	}

	// see if a minute has passed, if so advance the pointer to track the last hour
	if (uptime() >= lastday + 86400)
	{
		int16_t tmp;
		median_getbyindex(&PowerHours, 1, &tmp);
		pLastDay += tmp;
		median_add(&PowerDays, (int16_t)pLastDay / 24);
		pLastMin = 0;
		lastday = uptime();
	}


}