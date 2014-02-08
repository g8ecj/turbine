//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks
//
//
//  ui.c   -   User interface - drives the LCD and scans the keyboard for user input
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


#include <cfg/debug.h>

#include <cpu/irq.h>
#include <cpu/power.h>
#include <cpu/pgm.h>
#include <avr/eeprom.h>

#include <stdlib.h>

#include <algo/crc8.h>

#include <drv/timer.h>
#include <drv/ser.h>
#include <drv/lcd_hd44.h>
#include <drv/term.h>

#include "features.h"

#if PUSHBUTTONS == 1
#include <drv/kbd.h>
#endif

#include <drv/ow_1wire.h>
#include <drv/ow_ds2438.h>
#include <drv/ow_ds2413.h>

#include <avr/eeprom.h>

#include "control.h"
#include "measure.h"
#include "rpm.h"
#include "tlog.h"
#include "rtc.h"
#include "eeprommap.h"
#include "ui.h"


#define DEGREE 1
#define SDCARD 2
static const char lcd_degree[8] = { 0x1c, 0x14, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00 };	/* degree - char set B doesn't have it!! */
static const char lcd_sdcard[8] = { 0x1f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12, 0x1c };	/* sd card - bent rectangle! */

char degreestr[] = { DEGREE, 'C', 0 };


// a table of fields that are flashing
#define MAXFLASH    10
static int8_t flashing[MAXFLASH];


// Timings (in mS) for various activities
#define CAROSEL 5000L
#define BACKLIGHT 5000L
#define REFRESH 300L
#define FLASHON 700L
#define FLASHOFF 300L

extern Serial serial;
static Term term;

int16_t gUSdate;

typedef int8_t (*IncFunc_t) (int8_t field, int8_t dirn);


typedef struct vars
{
	int16_t *value;
	int16_t min;
	int16_t max;
	int16_t defval;
	uint8_t style;
	IncFunc_t get_inc;              // pointer to field increment function for this field
} Vars;



// forward reference
Vars variables[eNUMVARS];
float gVoffset = 1.03;
ticks_t carosel_timer;

static int8_t
null_inc (int8_t field, int8_t dirn)
{
	(void) field;
	(void) dirn;
	return 0;
}

static int8_t
deca_inc (int8_t field, int8_t dirn)
{
	(void) dirn;
	(void) field;
	return 10;
}

static int8_t
int_inc (int8_t field, int8_t dirn)
{
	(void) dirn;
	(void) field;
	return 1;
}

static int8_t
var_inc (int8_t field, int8_t dirn)
{
	(void) dirn;
	if (*variables[field].value > 999)
		return 20;
	if (*variables[field].value > 99)
		return 5;

	return 1;
}

static int8_t
six_inc (int8_t field, int8_t dirn)
{
	(void) dirn;
	(void) field;
	return 6;
}

static int8_t
cal_inc (int8_t field, int8_t dirn)
{
	// only the calibration field uses this so cheat!
	// need direction!!
	(void) field;
	float offset;

	offset = 1.0 / (float) gVolts;
	switch (dirn)
	{
	case 1:
		gVoffset += offset;
		break;
	case -1:
		gVoffset -= offset;
		break;
	case 0:
		gVoffset = 1.0;
		break;
	}
	return 0;
}



// the way I'm going to display the int16_t value - with decimal places, fixed width etc
enum STYLE
{
	eNORMAL,
	eDATE,
	eLARGE,
	eDECIMAL,
	eTRILEAN
};


//   value,    min,      max, default, style,   increment function
// Note: Its only worth having (real) limits for those values that can be changed.
Vars variables[eNUMVARS] = {
	{NULL,     0,    0,    0,  eNORMAL, null_inc},       // dummy 1st entry
	{&gVolts , 0,    0,    0, eDECIMAL, null_inc},       // volts
	{&gCharge, 0,    0,    0,  eNORMAL, null_inc},       // charge
	{&gPower,  0,    0,    0,  eNORMAL, null_inc},       // power

	{&gAmps,   0,    0,    0, eDECIMAL, null_inc},       // amps
	{&gDump,   0,    0,    0,  eNORMAL, null_inc},       // shunt regulator (dump load)
	{&gRPM,    0,    0,    0,  eNORMAL, null_inc},       // rpm

	{&gMaxhour, 0, 0, 0, eNORMAL, null_inc},             // max hours
	{&gMaxday,  0, 0, 0, eNORMAL, null_inc},             // max day
	{&gMinhour, 0, 0, 0, eNORMAL, null_inc},             // min hours
	{&gMinday,  0, 0, 0, eNORMAL, null_inc},             // min day
	{(int16_t *) & gCCA, 0, 65535, 0, eLARGE, null_inc}, // total
	{(int16_t *) & gDCA, 0, 65535, 0, eLARGE, null_inc}, // used

	{&gTemp,    0,    0,   0, eDECIMAL, null_inc},       // temperature

	{&gVlower, 2000, 2400, ddVlower, eDECIMAL, deca_inc},           // volt limit low
	{&gVupper, 2600, 3200, ddVupper, eDECIMAL, deca_inc},           // volt limit high
	{&gFloatVolts, 2600, 3200, ddFloatVolts, eDECIMAL, deca_inc},   // shunt volt low
	{&gAbsorbVolts, 2600, 3200, ddAbsorbVolts, eDECIMAL, deca_inc}, // shunt volt high

	{&gBankSize, 10, 9999, ddBankSize, eNORMAL, var_inc},           // charge high
	{&gMinCharge, 10, 9999, ddMinCharge, eNORMAL, var_inc},         // charge low
	{&gMaxCharge, 10, 9999, ddMaxCharge, eNORMAL, var_inc},         // charge high
	{&gMaxDischarge, 0, 99, 0, eNORMAL, var_inc},                   // number of discharge cycles
	{&gCharge, 0, 9999, ddCharge, eNORMAL, var_inc},                // sync

	{&gVoltage, 6, 48, ddVoltage, eNORMAL, six_inc},                // system voltage
	{&gVolts,   0,  0,  0, eDECIMAL, cal_inc},                      // calibrate

	{&gHOUR, 0, 23, ddHOUR, eDATE, int_inc},                        // hour
	{&gMINUTE, 0, 59, ddMINUTE, eDATE, int_inc},                    // minute
	{&gSECOND, 0, 59, ddSECOND, eDATE, int_inc},                    // second
	{&gDAY, 1, 31, ddDAY, eDATE, int_inc},                          // day
	{&gMONTH, 1, 12, ddMONTH, eDATE, int_inc},                      // month
	{&gYEAR, 12, 99, ddYEAR, eDATE, int_inc},                       // year

	{&gInverter, 0, 2, ddInverter, eTRILEAN, int_inc},              // control active
	{&gLoad, 0, 1, ddLoad, eTRILEAN, int_inc},                      // manual on/off

	{&gShunt, 0, 9999, ddShunt, eNORMAL, int_inc},                  // shunt conductance in Siemens
	{&gPoles, 0, 99, ddPoles, eNORMAL, int_inc},                    // magnetic poles in generator
	{&gSelfDischarge, 1, 90, ddSelfDischarge, eNORMAL, int_inc},    // battery leakage in days for 1% loss
	{&gIdleCurrent, 0, 999, ddIdleCurrent, eDECIMAL, int_inc},      // idle current of controller, router etc
	{&gAdjustTime, -719, 719, ddAdjustTime, eNORMAL, int_inc},      // clock adjuster
	{&gUSdate, 0, 1, ddUsdate, eTRILEAN, int_inc},                  // date format
};


Vars daymonth[2] = {
	{&gDAY, 1, 31, ddDAY, eDATE, int_inc},                // day
	{&gMONTH, 1, 12, ddMONTH, eDATE, int_inc},               // month
};



typedef struct screen
{
	int8_t field;                // global field number (relevant across all screens).
	// -1 = no value (text only)
	// -2 = end of array of structs
	int8_t row;                  // row of where to start text
	int8_t col;                  // column of where to start text
	const char *text;            // the text!!
	int8_t vcol;                 // the column of where to display the value
	int8_t width;                // width of the field
} Screen;


Screen screen1[] = {
	{eVOLTS, 0, 0, "Volts   ", 8, 6},
	{eCHARGE, 1, 0, "Charge        amp-hrs", 8, 4},
	{ePOWER, 2, 0, "Power          watts", 8, 5},
	{eAMPS, 3, 0, "Current         amps", 8, 6},
	{-2, 0, 0, "", 0, 0}
};


Screen screen2[] = {
	{eDUMP, 0, 0, "Dump       %", 8, 3},
	{eRPM, 1, 0, "Speed         RPM", 8, 3},
	{eTEMPERATURE, 2, 0, "Temp", 8, 6},
	{-1, 2, 15, degreestr, 0, 0},
	{eHOUR, 3, 0, "  :", 0, 2},
	{eMINUTE, 3, 3, "  :", 3, 2},
	{eSECOND, 3, 6, "", 6, 2},
	{eDAY, 3, 11, "  -", 11, 2},
	{eMONTH, 3, 14, "  -", 14, 2},
	{eYEAR, 3, 17, "", 17, 2},
	{-2, 0, 0, "", 0, 0}
};


Screen screen3[] = {
	{-1, 0, 3, "Max/Min/Total", 0, 0},
	{eMAXHOUR, 1, 0, "Hr ", 4, 5},
	{eMAXDAY, 1, 10, "Day ", 14, 5},
	{eMINHOUR, 2, 0, "Hr ", 4, 5},
	{eMINDAY, 2, 10, "Day ", 14, 5},
	{eTOTAL, 3, 0, "In     ", 4, 5},
	{eUSED, 3, 10, "Out    ", 14, 5},
	{-2, 0, 0, "", 0, 0}
};


Screen system[] = {
	{-1, 0, 3, "System", 0, 0},
	{eSYSTEM_VOLTS, 1, 0, "Voltage     ", 10, 5},
	{eCAL_VOLTS, 2, 0, "Calibrate   ", 10, 6},
	{eHOUR, 3, 0, "  :", 0, 2},
	{eMINUTE, 3, 3, "  :", 3, 2},
	{eSECOND, 3, 6, "", 6, 2},
	{eDAY, 3, 11, "  -", 11, 2},
	{eMONTH, 3, 14, "  -", 14, 2},
	{eYEAR, 3, 17, "", 17, 2},
	{-2, 0, 0, "", 0, 0}
};


Screen setup1[] = {
	{-1, 0, 3, "Voltage", 0, 0},
	{eVOLT_LIMIT_LO, 1, 0, "Min", 4, 5},
	{eVOLT_LIMIT_HI, 1, 10, "Max", 14, 5},
	{eVOLT_FLOAT,  2, 0, "Float", 10, 5},
	{eVOLT_ABSORB, 3, 0, "Absorb", 10, 5},
	{-2, 0, 0, "", 0, 0}
};


Screen setup2[] = {
	{-1, 0, 3, "Battery", 0, 0},
	{eBANK_SIZE, 1, 0, "Bank Size", 12, 4},
	{eMIN_CHARGE, 2, 0, "Min", 4, 4},
	{eMAX_CHARGE, 2, 10, "Max", 15, 4},
	{eDISCHARGE, 3, 0, "Cycle", 6, 2},
	{eSYNC, 3, 10, "Sync", 15, 4},
	{-2, 0, 0, "", 0, 0}
};

Screen setup3[] = {
	{-1, 0, 3, "Miscellaneous", 0, 0},
	{eSHUNT, 1, 0, "Shunt", 6, 4},
	{ePOLES, 1, 11, "Poles", 17, 2},
	{eSELFDISCHARGE, 2, 0, "Leak", 6, 3},
	{eIDLE_CURRENT, 2, 10, "Idle", 15, 5},
	{eADJUSTTIME, 3, 0, "Time", 6, 4},
	{eUSDATE, 3, 10, "Date", 15, 3},
	{-2, 0, 0, "", 0, 0}
};


Screen control[] = {
	{-1, 0, 3, "Control", 0, 0},
	{eINVERTER, 1, 0, "Inverter", 14, 4},
	{eMANUAL, 2, 0, "Override", 14, 4},
	{-2, 0, 0, "", 0, 0}
};


#define NUM_INFO 3
#define NUM_SETUPS  5
#define MAXSCREENS  NUM_INFO + NUM_SETUPS

static Screen *screen_list[] = { screen1, screen2, screen3, system, setup1, setup2, setup3, control };


static void set_month_day(uint8_t us)
{

	if (us)
	{
		variables[eDAY] = daymonth[1];
		variables[eMONTH] = daymonth[0];
	}
	// if already in US Date mode but want EURO mode then swap back
	else
	{
		variables[eDAY] = daymonth[0];
		variables[eMONTH] = daymonth[1];
	}
}

void get_month_day (uint8_t *month, uint8_t *day)
{

	*month = (uint8_t)*variables[eMONTH].value;
	*day = (uint8_t)*variables[eDAY].value;

}




void set_flash(int8_t field, int8_t set)
{
	int8_t i;

	for (i = 0; i < MAXFLASH; i++)
	{
		if (set)
		{
			// find a free slot or already set then set it
			if ((flashing[i] == 0) || (flashing[i] == field))
			{
				flashing[i] = field;
				break;
			}
		}
		else
		{
			// find which slot its in and clear it
			if (flashing[i] == field)
			{
				flashing[i] = 0;
				break;
			}
		}
	}
}

// return an indicator on whether this field is flashing and should currently be blanked (true) or displayed (false)
static int8_t check_flash(int8_t field)
{
	int8_t i;
	static int8_t flash_state = true;
	static ticks_t flash_on_timer, flash_off_timer;

	// always toggle flash_state with the correct cadence then see if we need it!!
	if (flash_state)                  // currently on, see if on time has expired
	{
		if (timer_clock () - flash_off_timer > ms_to_ticks (FLASHOFF))
		{
			// timer expired, set off timer and turn off (blank) field
			flash_on_timer = timer_clock ();
			flash_state = false;          // signify its now displayed
		}
	}
	else                           // currently on, turn it off
	{
		if (timer_clock () - flash_on_timer > ms_to_ticks (FLASHON))
		{
			flash_off_timer = timer_clock ();
			flash_state = true;          // signify its now blanked out
		}
	}

	for (i = 0; i < MAXFLASH; i++)
	{
		if (flashing[i] == field)
			return flash_state;
	}
	return false;
}

// display a variable or blanks of the correct length at the coordinates for this field in this screen
static void print_field(int16_t value, int8_t field, uint8_t screen)
{
	int8_t i;
	int16_t whole, part;
	char spaces[10] = "         ";
	char tritext[4][5] = {"off", "on", "auto", "oops" };

	Screen *scrn = screen_list[screen];

	for (i = 0; scrn[i].field != -2; i++)
	{
		if (scrn[i].field == field)	// found the correct one
		{
			kfile_printf (&term.fd, "%c%c%c", TERM_CPC, TERM_ROW + scrn[i].row, TERM_COL + scrn[i].vcol);
			kfile_printf (&term.fd, "%.*s", scrn[i].width, spaces);
			if (check_flash (field))
				break;
			kfile_printf (&term.fd, "%c%c%c", TERM_CPC, TERM_ROW + scrn[i].row, TERM_COL + scrn[i].vcol);
			switch (variables[field].style)
			{
			case eNORMAL:
				kfile_printf (&term.fd, "%d", value);
				break;
			case eDATE:
				kfile_printf (&term.fd, "%02d", value);
				break;
			case eLARGE:
				kfile_printf (&term.fd, "%u", (uint16_t) value);
				break;
			case eDECIMAL:
				// split the value into those bits before and after the decimal point
				// if the whole part is less than 1 then we loose the sign bit so do it manually in all cases
				whole = abs (value / 100);
				part = abs (value % 100);
				kfile_printf (&term.fd, "%.*s%d.%02u", value < 0 ? 1 : 0, "-",  whole, part);
				break;
			case eTRILEAN:
				kfile_printf (&term.fd, "%s", tritext[value & 3]);
				break;
			}
			break;
		}
	}
}


// scan screen to determine the min and max field numbers
// direction can be -1 for up, 1 for down or 0 for current field
// min & max handle the wrap round
// return the new field
// assumes fields are contiguous on a screen

static int8_t find_next_field (int8_t field, int8_t screen, int8_t dirn)
{
	int8_t i, min = 99, max = -1;
	Screen *scrn = screen_list[screen];

	for (i = 0; scrn[i].field != -2; i++)
	{
		if (scrn[i].field < 0)
			continue;
		if (scrn[i].field > max)
			max = scrn[i].field;
		if (scrn[i].field < min)
			min = scrn[i].field;
	}

	field += dirn;
	if (field > max)
		field = min;
	if (field < min)
		field = max;

	return field;
}

// find out what line this field is on
static int8_t get_line(int8_t field, int8_t screen)
{
	Screen *scrn = screen_list[screen];
	int8_t i;

	// get the line this field is on
	for (i = 0; scrn[i].field != -2; i++)
	{
		if (scrn[i].field < 0)
			continue;
		if (field == scrn[i].field)
		{
			return scrn[i].row;
		}
	}
	// if field not found then return something odd!!
	return -1;
}

// find the next line by scanning fields in the direction requested
static int8_t find_next_line (int8_t field, int8_t screen, int8_t dirn)
{
	int8_t startline, line = 0;
	int8_t startfield = field;

	startline = get_line(field, screen);

	while (1)
	{
		// move fields in the direction specified
		field = find_next_field (field, screen, dirn);
		line = get_line(field, screen);
		// moved to another line
		if (line != startline)
			break;
		// if only 1 line then wrapped back to where we started
		if (field == startfield)
			break;
	}

	return field;

}


// display the text and optional field for all lines on a screen
static void print_screen (int8_t screen)
{
	int8_t i = 0;
	Screen *scrn = screen_list[screen];
	char tmp[4];

	kfile_printf (&term.fd, "%c", TERM_CLR);
	while (scrn[i].field != -2)
	{
		kfile_printf (&term.fd, "%c%c%c", TERM_CPC, TERM_ROW + scrn[i].row, TERM_COL + scrn[i].col);
		kfile_printf (&term.fd, "%s", scrn[i].text);

		if (scrn[i].field != -1)
		{
			print_field (*variables[scrn[i].field].value, scrn[i].field, screen);
		}

		i++;
	}
	// indicate there is an sd card plugged in (or not!!) with icon or space
	tmp[0] = gLoad ? 'I' : 0x20;
	tmp[1] = charge_mode;
	tmp[2] = sd_ok ? SDCARD : 0x20;
	tmp[3] = 0;
	kfile_printf (&term.fd, "%c%c%c%s", TERM_CPC, TERM_ROW + 0, TERM_COL + 17, tmp);

}




// scan through a few variables and check their limits to see if they should be flashing
static void flag_warnings(void)
{
	if ((gVolts < gVlower) || (gVolts > gAbsorbVolts))
		set_flash(eVOLTS, true);
	else
		set_flash(eVOLTS, false);

	if (gCharge < gMinCharge)
		set_flash(eCHARGE, true);
	else
		set_flash(eCHARGE, false);

}

// used externally to verify that a value is within the correct range and set it if so.
bool check_value(enum VARS var, int16_t value)
{
	if ((value >= variables[var].min) && (value <= variables[var].max))
	{
		*variables[var].value = value;
		return false;
	}
	else
		return true;

}

// initialise the module!
void ui_init (void)
{

	lcd_hw_init ();
	lcd_display (1, 0, 0);
	lcd_remapChar (lcd_degree, DEGREE);        // put the degree symbol on character 0x01
	lcd_remapChar (lcd_sdcard, SDCARD);        // put the sd card symbol on character 0x02
	term_init (&term);
#if PUSHBUTTONS == 1
	kbd_init();
	kbd_setRepeatMask(K_UP | K_DOWN);
#endif
	carosel_timer = timer_clock ();
	set_month_day(gUSdate);

}




// mode values
#define SETUP       1
#define MONITOR     2
#define PAGEEDIT    3
#define FIELDEDIT   4




void run_ui (void)
{
	static int8_t screen_number = 0, field = 0, mode = MONITOR;
	static ticks_t backlight_timer, refresh_timer;
	static int16_t working_value;

	flag_warnings();

#if PUSHBUTTONS == 1
	keymask_t key;
	key = kbd_peek();
#else
	int16_t key;
#define K_UP          'u'
#define K_DOWN        'd'
#define K_LEFT        'l'
#define K_RIGHT       'r'
#define K_CENTRE      'c'
	key = kfile_getc (&serial.fd);
	if (key == EOF)
		key = 0;
#endif

	// if key pressed then ignite backlight for a short while
	if (key)
	{
		lcd_bl_on();
		backlight_timer = timer_clock ();
		key &= K_CENTRE | K_RIGHT | K_LEFT | K_UP | K_DOWN;
	}
	else
	{
		if (timer_clock () - backlight_timer > ms_to_ticks (BACKLIGHT))
		{
			lcd_bl_off();
		}
	}

	if (timer_clock () - refresh_timer > ms_to_ticks (REFRESH))
	{
		refresh_timer = timer_clock ();
		print_screen (screen_number);
	}


	switch (mode)
	{
	case FIELDEDIT:
		// the working value during calibration needs to be updated from the real value
		if (field == eCAL_VOLTS)
			working_value = gVolts;
		// refresh the value to place the cursor on the screen in the right place
		print_field (working_value, field, screen_number);

		switch (key)
		{
			int16_t inc;
		case K_CENTRE:
			// save value and exit field edit mode
			// save the working value into the real one
			*variables[field].value = working_value;
			// some fields require special action
			switch (field)
			{
			case eCAL_VOLTS:
				// turn off backlight to reduce current before calibration
				lcd_bl_off();
				do_calibration ();
				break;
			case eSYNC:
				set_charge (working_value);
				break;
			case eHOUR:
			case eMINUTE:
			case eSECOND:
			case eDAY:
			case eMONTH:
			case eYEAR:
			case eADJUSTTIME:
				// set Unix time in seconds, save adjustment in eeprom
				set_epoch_time ();
				break;
			case eUSDATE:
				set_month_day(gUSdate);
				break;
			case eMANUAL:
				if (gLoad == LOADOFF)
					do_command (MANUALON);
				else
					do_command (MANUALOFF);
				break;
			}
			save_eeprom_values();

			mode = PAGEEDIT;
			set_flash(field, false);
			break;
		case K_UP:
			/// increase by increment
			inc = variables[field].get_inc (field, 1);
			if (working_value + inc <= variables[field].max)
				working_value += inc;
			else              // wrap
				working_value = variables[field].min;
			break;
		case K_DOWN:
			// decrease by increment
			inc = variables[field].get_inc (field, -1);
			if (working_value - inc >= variables[field].min)
				working_value -= inc;
			else               // wrap
				working_value = variables[field].max;
			break;
		case K_LEFT:
			mode = PAGEEDIT;
			// abort - reload previous values
			load_eeprom_values();
			set_flash(field, false);               // make sure flash is off
			break;
		case K_RIGHT:
			// inc of zero special case for voltage calibration to set default
			variables[field].get_inc (field, 0);
			// load default
			working_value = variables[field].defval;
			break;
		}
		break;


// centre enters field edit mode
// left/right together exits field navigation and moves round setup screens
	case PAGEEDIT:
		// refresh the value to place the cursor on the screen in the right place
		print_field (*variables[field].value, field, screen_number);
		switch (key)
		{
		case K_CENTRE:
			// enter this field to change it
			mode = FIELDEDIT;
			set_flash(field, true);
			// refresh the value
			working_value = *variables[field].value;
			break;
		case K_LEFT | K_RIGHT:
			// exit edit mode
			// turn off cursor
			kfile_printf (&term.fd, "%c", TERM_BLINK_OFF);
			mode = SETUP;
			break;
		case K_UP:
			// field on previous line
			field = find_next_line (field, screen_number, -1);
			break;
		case K_DOWN:
			// field on next line
			field = find_next_line (field, screen_number, 1);
			break;
		case K_LEFT:
			// previous field
			field = find_next_field (field, screen_number, -1);
			break;
		case K_RIGHT:
			// next field
			field = find_next_field (field, screen_number, 1);
			break;
		}
		break;


// up/down moves round monitor screens
// left right moves round setup screens
// when in setup screen then centre enters field navigation mode
	case SETUP:
		switch (key)
		{
		case K_CENTRE:
			// enter edit mode
			mode = PAGEEDIT;
			// turn on cursor
			kfile_printf (&term.fd, " %c", TERM_BLINK_ON);
			// get the first field on this screen and display it
			field = find_next_field (0, screen_number, 0);
			print_field(*variables[field].value, field, screen_number);
			break;
		case K_RIGHT:
			screen_number++;
			if (screen_number >= MAXSCREENS)
				screen_number = NUM_INFO;
			print_screen (screen_number);
			break;
		case K_LEFT:
			screen_number--;
			if (screen_number < NUM_INFO)
				screen_number = MAXSCREENS - 1;
			print_screen (screen_number);
			break;
		case K_UP:
			mode = MONITOR;
			screen_number = NUM_INFO - 1;
			print_screen (screen_number);
			break;
		case K_DOWN:
			mode = MONITOR;
			screen_number = 0;
			print_screen (screen_number);
			break;
		}
		break;


// up/down moves round monitor screens
// left right moves round setup screens
// when in monitor screen then centre toggles carosel mode
	case MONITOR:
		switch (key)
		{
		case K_CENTRE:
			if (carosel_timer == 0)
			{
				carosel_timer = timer_clock ();
				screen_number = 0;
				print_screen (screen_number);
			}
			else
				carosel_timer = 0;
			break;
		case K_UP:
			screen_number = (screen_number + 1) % NUM_INFO;
			print_screen (screen_number);
			break;
		case K_DOWN:
			screen_number = (screen_number - 1 + NUM_INFO) % NUM_INFO;
			print_screen (screen_number);
			break;
		case K_LEFT:
			mode = SETUP;
			screen_number = MAXSCREENS - 1;
			print_screen (screen_number);
			break;
		case K_RIGHT:
			mode = SETUP;
			screen_number = NUM_INFO;
			print_screen (screen_number);
			break;
		}
		if (carosel_timer && timer_clock () - carosel_timer > ms_to_ticks (CAROSEL))
		{
			carosel_timer = timer_clock ();
			refresh_timer = timer_clock ();
			print_screen (screen_number);
			if (++screen_number >= (int8_t) NUM_INFO)
				screen_number = 0;
		}
		break;

	}

}
