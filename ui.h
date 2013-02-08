//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks
//
//
//  ui.h   -   User interface - drives the LCD and scans the keyboard for user input
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

#ifndef _UI_H
#define _UI_H

// all the fields displayed
enum VARS
{
	eVOLTS = 1,
	eCHARGE,
	ePOWER,

	eAMPS,
	eDUMP,
	eRPM,

	eMAXHOUR,
	eMAXDAY,
	eMINHOUR,
	eMINDAY,
	eTOTAL,
	eUSED,

	eTEMPERATURE,

	eVOLT_LIMIT_LO,
	eVOLT_LIMIT_HI,
	eVOLT_FLOAT,
	eVOLT_ABSORB,

	eBANK_SIZE,
	eMIN_CHARGE,
	eSYNC,

	eSYSTEM_VOLTS,
	eCAL_VOLTS,
	eHOUR,
	eMINUTE,
	eSECOND,
	eDAY,
	eMONTH,
	eYEAR,

	eINVERTER,
	eMANUAL,

	eSHUNT,
	ePOLES,
	eSELFDISCHARGE,
	eIDLE_CURRENT,
	eADJUSTTIME,
	eUSDATE,
	eNUMVARS
};



void ui_init(void);
void run_ui(void);
void set_flash(int8_t field, int8_t set);
bool check_value(enum VARS var, int16_t value);
void load_eeprom_values(void);
void get_month_day (uint8_t *month, uint8_t *day);

#endif

