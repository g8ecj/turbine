//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  rtc.h   -   Real time clock - provides date, time and seconds since 1st Jan 1970
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


extern int16_t gSECOND;
extern int16_t gMINUTE;
extern int16_t gHOUR;
extern int16_t gDAY;
extern int16_t gMONTH;
extern int16_t gYEAR;
extern int16_t gAdjustTime;


//  Function declarations
void rtc_init (void);           //initialize the Timer Counter 2 in asynchron operation
void run_rtc (void);        //updates the time and date
// time in seconds since midnight, 1st Jan 2000
uint32_t time(void);
void set_epoch_time(void);
void get_datetime(uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* min, uint8_t* sec);

