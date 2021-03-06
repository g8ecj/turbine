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
#include <drv/lcd_hd44.h>
#include <drv/term.h>
#include <drv/ow_ds2438.h>
#include <drv/ow_ds2413.h>
#include <drv/ow_ds18x20.h>
#include "io/kfile.h"

#include "tlog.h"
#include <cfg/log.h>

#include "features.h"
#include "median.h"
#include "graph.h"
#include "rtc.h"
#include "eeprommap.h"
#include "measure.h"



MEDIAN PowerMins;
MEDIAN PowerHours;
MEDIAN PowerDays;

static const char lcd_botquar[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f };
static const char lcd_bothalf[8] = {0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f };
static const char lcd_botthre[8] = {0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
static const char lcd_topthre[8] = {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x00 };
static const char lcd_tophalf[8] = {0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00 };
static const char lcd_topquar[8] = {0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const char lcd_block[8]   = {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };

char graphmap[4][17] = {
{0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, BOTQUAR, BOTHALF, BOTTHRE, BLOCK, BLOCK },
{0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, BOTQUAR, BOTHALF, BOTTHRE, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK },
{BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, TOPTHRE, TOPHALF, TOPQUAR, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
{BLOCK, TOPTHRE, TOPHALF, TOPQUAR, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 }};


void
graph_init (void)
{
	uint8_t i;

	// read old days info from eeprom and if it look wrong, initialise it to zero
	eeprom_read_block ((void *) &PowerDays, (const void *) &eePowerDays, sizeof (PowerDays));
	if (median_getSize(&PowerDays) != 20)
	{
		median_init(&PowerDays, 20);
		for (i = 0; i < 20; i++)
			median_add(&PowerDays, 0);

		eeprom_write_block ((const void *) &PowerDays, (void *) &eePowerDays, sizeof (PowerDays));
	}

	// do the same with hours
	eeprom_read_block ((void *) &PowerHours, (const void *) &eePowerHours, sizeof (PowerHours));
	if (median_getSize(&PowerHours) != 20)
	{
		median_init(&PowerHours, 20);
		for (i = 0; i < 20; i++)
			median_add(&PowerHours, 0);

		eeprom_write_block ((const void *) &PowerHours, (void *) &eePowerHours, sizeof (PowerHours));
	}

	median_init(&PowerMins, 20);
	for (i = 0; i < 20; i++)
	{
		median_add(&PowerMins, 0);
	}

	lcd_remapChar (lcd_botquar, BOTQUAR);
	lcd_remapChar (lcd_bothalf, BOTHALF);
	lcd_remapChar (lcd_botthre, BOTTHRE);
	lcd_remapChar (lcd_topthre, TOPTHRE);
	lcd_remapChar (lcd_tophalf, TOPHALF);
	lcd_remapChar (lcd_topquar, TOPQUAR);
	lcd_remapChar (lcd_block,   BLOCK);
}


void
run_graph (void)
{
	static uint32_t lastsec = 0, lastmin = 0, lasthour = 0, lastday = 0;
	static int32_t pLastMin = 0, pLastHour = 0, pLastDay = 0;
	static uint8_t mincount = 0;


	// see if a second has passed, if so add power into minute accumulator
	if (uptime() >= lastsec + 1)
	{
		pLastMin += gPower;
		lastsec = uptime();
	}

	// see if a minute has passed, if so advance the pointer to track the last 60 minutes
	if (uptime() >= lastmin + 60)
	{
		pLastHour += pLastMin / 60;
		if (++mincount >= 3)
		{
			median_add(&PowerMins, (int16_t)(pLastMin / 60));
			mincount = 0;
		}
		pLastMin = 0;
		lastmin = uptime();
	}

	// see if an hour has passed, if so advance the pointer to track the last 20 of them
	if (uptime() >= lasthour + 3600)
	{
		pLastDay += pLastHour / 60;
		median_add(&PowerHours, (int16_t)(pLastHour / 60));
		eeprom_write_block ((const void *) &PowerHours, (void *) &eePowerHours, sizeof (PowerHours));
		pLastHour = 0;
		lasthour = uptime();
	}

	// see if a day has passed, if so advance the pointer to track the last 20 days
	if (uptime() >= lastday + 86400)
	{
		// track daily totals in watt-hours
		median_add(&PowerDays, (int16_t)pLastDay);
		eeprom_write_block ((const void *) &PowerDays, (void *) &eePowerDays, sizeof (PowerDays));
		pLastDay = 0;
		lastday = uptime();
	}
}

void
print_graph (KFile *stream, uint8_t type, uint8_t style)
{
	MEDIAN *mArray;
	uint8_t i, j, index;
	int16_t highest, lowest, scale, value;
	static uint8_t last_type = 99, last_style = 99;
	char buf[21];

	switch(type)
	{
	case MINGRAPH:
		mArray = &PowerMins;
		strncpy(buf, "Hourly", 7);
		break;
	case HOURGRAPH:
		mArray = &PowerHours;
		strncpy(buf, "Daily", 6);
		break;
	case DAYGRAPH:
		mArray = &PowerDays;
		strncpy(buf, "Monthly", 8);
		break;
	default:
		mArray = &PowerMins;
		break;
	}

	median_getHighest(mArray, &highest);
	median_getLowest(mArray, &lowest);
	scale = MAX(abs(highest), abs(lowest));

	if ((last_type != type) || (last_style != style))
	{
		kfile_putc(TERM_CLR, stream);
		last_type = type;
		last_style = style;
	}

	if ((style == GRAPHSTYLE) && scale)
	{
		for (i = 0; i < 4; i++)          // for each line to be displayed
		{
			index = median_getStart(mArray);
			for (j = 0; j < median_getCount(mArray); j++)
			{
				median_getNext(mArray, &index, &value);
				buf[j] = graphmap[i][(int32_t)(value + scale) * 8 / scale];
			}
			buf[j] = 0;
			kfile_printf(stream, "%c%c%c%s", TERM_CPC, TERM_ROW + i, TERM_COL, buf);
		}
	}
	else
	{
		kfile_printf(stream, "\r\n%s Range:\r\n    %d to %d", buf, lowest, highest);
	}

}


