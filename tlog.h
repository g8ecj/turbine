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

#define LOG_LEVEL LOG_LVL_INFO

// event values
#define LOG_OVERVOLT    0x01
#define LOG_UNDERVOLT   0x02
#define LOG_MANUALON    0x03
#define LOG_MANUALOFF   0x04
#define LOG_CHARGED     0x05
#define LOG_DISCHARGED  0x06
#define LOG_SHUNTON     0x07
#define LOG_SHUNTOFF    0x08
#define LOG_MARKTIME    0x09
#define LOG_NEWHOURMAX  0x0a
#define LOG_NEWDAYMAX   0x0b
#define LOG_NEWHOURMIN  0x0c
#define LOG_NEWDAYMIN   0x0d
#define LOG_LEAKADJUST  0x0e

// bit flags
#define LOG_ERROR       0x10
#define LOG_SHUNT       0x40
#define LOG_LOAD        0x80
#define LOG_NULL        0x00    // no events!! manual display!
