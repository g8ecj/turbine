
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

static void sort(MEDIAN *M) 
{
	// copy
	for (uint8_t i=0; i< M->cnt; i++) M->as[i] = M->ar[i];

	// sort all
	for (uint8_t i=0; i< M->cnt-1; i++) {
		uint8_t m = i;
		for (uint8_t j=i+1; j< M->cnt; j++) {
			if (M->as[j] < M->as[m]) m = j;
		}
		if (m != i) {
			int16_t t = M->as[m];
			M->as[m] = M->as[i];
			M->as[i] = t;
		}
	}
}



void median_init(MEDIAN *M,  uint8_t size)
{
	M->size = constrain(size, MIN_MEDIAN, MAX_MEDIAN);
	M->cnt = 0;
	M->idx = 0;
	memset(M->ar, 0, M->size);
}


void median_clear(MEDIAN *M) 
{
	M->cnt = 0;
	M->idx = 0;
}

void median_add(MEDIAN *M, int16_t value)
{
	M->ar[M->idx++] = value;
	if (M->idx >= M->size) M->idx = 0; // wrap around
	if (M->cnt < M->size) M->cnt++;
}

bool median_getMedian(MEDIAN *M, int16_t *value)
{
	if (M->cnt > 0) {
		sort(M);
		*value = M->as[M->cnt/2];
		return OK;
	}
	return NOK;
}

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

bool median_getHighest(MEDIAN *M, int16_t *value) 
{
	if (M->cnt > 0) {
		sort(M);
		*value = M->as[M->cnt-1];
		return OK;
	}
	return NOK;
}

bool median_getLowest(MEDIAN *M, int16_t *value) 
{
	if (M->cnt > 0) {
		sort(M);
		*value =  M->as[0];
		return OK;
	}
	return NOK;
}

uint8_t median_getSize(MEDIAN *M) 
{
	return M->size;
}

uint8_t median_getCount(MEDIAN *M) 
{
	return M->cnt;
}

bool median_getStatus(MEDIAN *M) 
{
	return (M->cnt > 0 ? OK : NOK);
}


// --- END OF FILE ---
