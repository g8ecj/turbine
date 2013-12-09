
#ifndef _MEDIAN_H
#define _MEDIAN_H

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

#include <stdint.h>
#include <stdbool.h>


#define MIN_MEDIAN 1
#define MAX_MEDIAN 61

#define OK true
#define NOK false

typedef struct running_median {
   int16_t ar[MAX_MEDIAN];
   uint8_t idx;
   uint8_t cnt;
   uint8_t size;
} MEDIAN;


void median_init(MEDIAN *M,  uint8_t size);
void median_clear(MEDIAN *M);
void median_add(MEDIAN *M, int16_t value);
bool median_getMedian(MEDIAN *M, int16_t * value);
bool median_getAverage(MEDIAN *M, int16_t *value);
bool median_getHighest(MEDIAN *M, int16_t *value);
bool median_getLowest(MEDIAN *M, int16_t *value);
uint8_t median_getSize(MEDIAN *M);
uint8_t median_getCount(MEDIAN *M);
bool median_getStatus(MEDIAN *M);
void median_tik(MEDIAN *M);
void median_addmin(MEDIAN *M, int16_t value);
void median_addmax(MEDIAN *M, int16_t value);



#endif
// --- END OF FILE ---