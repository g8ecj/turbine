//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks
//
//
//  tlog.h   -   This program logs data to dataflash on an ATMEL  Butterfly board
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

#include <stdint.h>
#include <stdbool.h>

void log_init (void);
void run_log (void);
void log_event (uint8_t event);
void log_clear (void);

extern bool sd_ok;

#define LOG_LEVEL LOG_LVL_WARN

// event values
#define LOG_OVERVOLT    1
#define LOG_UNDERVOLT   2
#define LOG_MANUALON    3
#define LOG_MANUALOFF   4
#define LOG_CHARGED     5
#define LOG_DISCHARGED  6
#define LOG_SHUNTON     7
#define LOG_SHUNTOFF    8
#define LOG_MARKTIME    9
#define LOG_NEWHOURMAX  10
#define LOG_NEWDAYMAX   11
#define LOG_NEWHOURMIN  12
#define LOG_NEWDAYMIN   13
#define LOG_LEAKADJUST  14
#define LOG_MASK_VALUE  0x0f
// bit flags
#define LOG_ERROR       0x10
#define LOG_SHUNT       0x40
#define LOG_LOAD        0x80
#define LOG_NULL        0x00    // no events!! manual display!
