//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks
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

#ifndef _MINMAX_H
#define _MINMAX_H


#include <stdint.h>
#include <stdbool.h>


#define MIN_MINMAX 1
#define MAX_MINMAX 61

typedef struct running_minmax {
   int16_t data[MAX_MINMAX];
   uint8_t index;
   uint8_t size;
   bool minmax;
} MINMAX;

void minmax_init(MINMAX *MM, int16_t size, bool minmax);
void minmax_add (MINMAX *MM);
int16_t minmax_get (MINMAX *MM, int16_t value);

#endif
