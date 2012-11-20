//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks
//
//
//  minmax.c   -   This module provides a set of functions that allow min and max values over a time period to be determined
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
// 


#include "minmax.h"

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

//< \param MM pointer to a struct that holds the variables for this instance
//< \param size how large the min/max array is (seconds, minutes, hours etc)
//< \param minmax indicates whether we're finding the min or the max value over the period
void minmax_init(MINMAX *MM, int16_t size, bool minmax)
{
	int8_t j;
	
	MM->size = constrain(size, MIN_MINMAX, MAX_MINMAX);
	MM->minmax = minmax;
	MM->idx = 0;

	for (j = 0; j < MM->size; j++)
	{
		MM->data[j] = MM->minmax ? -32767: 32767;
	}

}


// Advance the internal pointers at the appropriate time
//< \param MM pointer to a struct that holds the variables for this instance
void minmax_add (MINMAX *MM)
{

	MM->idx++;
	if (MM->idx >= MM->size)
		MM->idx = 0;               // reset to the start of the array
	MM->data[MM->idx] = MM->minmax ? -32767: 32767;    // clear the next slot in the array

}


// Keep the current slot updated with the current value and return the min/max for the period
//< \param MM pointer to a struct that holds the variables for this instance
//< \param value New value to try adding
//< \return the current min or max value
int16_t minmax_get (MINMAX *MM, int16_t value)
{

	int16_t ret;
	int8_t j;

	if (MM->minmax)
	{
		// save the present value if greater than already there in the current slot
		if (value > MM->data[MM->idx])
			MM->data[MM->idx] = value;
		// find a new maximum and return it
		ret = -32767;
		for (j = 0; j < MM->size; j++)
			if (MM->data[j] > ret)
				ret = MM->data[j];
	}
	else
	{
		// save the present value if less than already there in the current slot
		if (value < MM->data[MM->idx])
			MM->data[MM->idx] = value;
		// find a new minimum and return it
		ret = 32767;
		for (j = 0; j < MM->size; j++)
			if (MM->data[j] < ret)
				ret = MM->data[j];
	}

	return ret;
}




