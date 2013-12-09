//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks (but see below for the original authors)
//
//
//  median.c   -   This module provides a set of functions that allow glitches to be detected and ignored.
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
//    FILE: RunningMedian.h
//  AUTHOR: Rob dot Tillaart at gmail dot com  
// PURPOSE: RunningMedian library for Arduino
// VERSION: 0.2.00 - template edition
//     URL: http://arduino.cc/playground/Main/RunningMedian
// HISTORY: 0.2.00 first template version by Ronny
//
// Released to the public domain
//


#include "median.h"
#include <string.h>

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

int16_t sorted[MAX_MEDIAN];

static void sort(MEDIAN *M) 
{
	// copy
	for (uint8_t i=0; i< M->cnt; i++) sorted[i] = M->ar[i];

	// sort all
	for (uint8_t i=0; i< M->cnt-1; i++) {
		uint8_t m = i;
		for (uint8_t j=i+1; j< M->cnt; j++) {
			if (sorted[j] < sorted[m]) m = j;
		}
		if (m != i) {
			int16_t t = sorted[m];
			sorted[m] = sorted[i];
			sorted[i] = t;
		}
	}
}


//< Initialise the buffering array and all variables for this instance
//< \param M pointer to a struct that holds the variables for this instance of median calculator
// < \param size number of entries in the array
void median_init(MEDIAN *M,  uint8_t size)
{
	M->size = constrain(size, MIN_MEDIAN, MAX_MEDIAN);
	M->cnt = 0;
	M->idx = 0;
	memset(M->ar, 0, M->size * sizeof(M->ar[0]));
}

//< Clears the variables used to build the median array
//< \param M pointer to a struct that holds the variables for this instance of median calculator
void median_clear(MEDIAN *M) 
{
	M->cnt = 0;
	M->idx = 0;
}


//< Adds a value to the median array, updates and wraps indices discarding expired data.
//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \param value 16 bit signed value to add to median array
void median_add(MEDIAN *M, int16_t value)
{
	M->ar[M->idx++] = value;
	if (M->idx >= M->size) M->idx = 0; // wrap around
	if (M->cnt < M->size) M->cnt++;
}

//< Sort the median array and return the centre of the sorted array
//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \param value pointer to value returned from the centre of the sorted median array
bool median_getMedian(MEDIAN *M, int16_t *value)
{
	if (M->cnt > 0) {
		sort(M);
		*value = sorted[M->cnt/2];
		return OK;
	}
	return NOK;
}

// Get the average of the value in the median array
//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \param value pointer to average value returned
bool median_getAverage(MEDIAN *M, int16_t *value) 
{
	if (M->cnt > 0) {
		float sum = 0;
		for (uint8_t i=0; i< M->cnt; i++) sum += M->ar[i];
		*value = sum / M->cnt;
		return OK;
	}
	return NOK;
}

//< Get the highest value by returning the end of the sorted median array
//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \param value pointer to value highest value returned
bool median_getHighest(MEDIAN *M, int16_t *value) 
{
	if (M->cnt > 0) {
		sort(M);
		*value = sorted[M->cnt-1];
		return OK;
	}
	return NOK;
}


//< Get the lowest value by returning the start of the sorted median array
//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \param value pointer to value lowest value returned
bool median_getLowest(MEDIAN *M, int16_t *value) 
{
	if (M->cnt > 0) {
		sort(M);
		*value =  sorted[0];
		return OK;
	}
	return NOK;
}


//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \return size of median array
uint8_t median_getSize(MEDIAN *M) 
{
	return M->size;
}

//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \return number of entries so far in the median array
uint8_t median_getCount(MEDIAN *M) 
{
	return M->cnt;
}

//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \return whether there are any entries at all in the median array
bool median_getStatus(MEDIAN *M) 
{
	return (M->cnt > 0 ? OK : NOK);
}

//< Increments pointers to the median array, wraps indices discarding expired data.
//< Used as a time tick to advance with array wrt time
//< \param M pointer to a struct that holds the variables for this instance of median calculator
void median_tik(MEDIAN *M)
{
	if (M->idx >= M->size) M->idx = 0; // wrap around
	if (M->cnt < M->size) M->cnt++;
	M->ar[M->idx] = 0x8000;
}

//< Overwrite the current value in the median array, if a new entry or less than current value
//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \param value 16 bit signed value to add to median array
void median_addmin(MEDIAN *M, int16_t value)
{
	if (M->ar[M->idx] == 0x8000)
		M->ar[M->idx] = value;
	if (value < M->ar[M->idx])
		M->ar[M->idx] = value;
}

//< Overwrite the current value in the median array, if a new entry or greater than current value
//< \param M pointer to a struct that holds the variables for this instance of median calculator
//< \param value 16 bit signed value to add to median array
void median_addmax(MEDIAN *M, int16_t value)
{
	if (M->ar[M->idx] == 0x8000)
		M->ar[M->idx] = value;
	if (value > M->ar[M->idx])
		M->ar[M->idx] = value;
}

