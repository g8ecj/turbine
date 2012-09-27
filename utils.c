//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  utils.c   -   This program smooths a int16 type through a buffer array
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
#include <stdlib.h>
#include <string.h>

#include "features.h"
#include "utils.h"


/*****************************************************************************
*
* smooth_init     -- smoothing function generates running average from n samples
* pointer to S_ARRAY structure
* number of samples to average (to max of MAXSAMPLES)
*
*****************************************************************************/


void
smooth_init (S_ARRAY * ptr, int16_t * hbuff, uint8_t size)
{
   ptr->history = hbuff;
   ptr->size = size;
   ptr->avptr = 0;
   ptr->avstart = 0;
   ptr->total = 0;
   memset(hbuff, 0, size);
}

/*****************************************************************************
*
* smooth     -- smoothing function generates running average from n samples
* pointer to S_ARRAY structure
* value to add to averaging
* returns average so far
*
*****************************************************************************/
int16_t
smooth (S_ARRAY * ptr, int16_t value)
{
   int16_t ret;
   // smoothing function using a running average in a history buffer
   // handy in the case where we misread a register
   ptr->history[ptr->avptr] = value;
   ptr->total += value;

   // advance pointer &  keep the history pointer within bounds 
   ptr->avptr++;
   if (ptr->avptr >= ptr->size)
      ptr->avptr = 0;

   // when we start we haven't filled the history buffer so adjust averaging algorithm 
   // to only count those values in the buffer 
   if (ptr->avstart < ptr->size)
      ptr->avstart++;

   // calculate the average based on the number of entries so far
   ret = ptr->total / ptr->avstart;

   // if we have got a full set of samples in the averaging buffer, drop the oldest one
   // we are already pointing at the oldest entry due to advancing and wrapping pointer above
   // assumes the buffer is clear at startup
   ptr->total -= ptr->history[ptr->avptr];

   return ret;

}

