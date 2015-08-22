//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks
//
//
//  tlog.c   -   This program logs data to dataflash on an ATMEL AVR Butterfly board
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

#include "tlog.h"


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/eeprom.h>

#include <io/kfile.h>

#include <drv/ser.h>
#include <drv/timer.h>
#include <cfg/log.h>
#include <drv/lcd_hd44.h>

#include "sd_raw.h"
#include "partition.h"
#include "fat.h"

#include "features.h"

#include "measure.h"
#include "control.h"
#include "rpm.h"
#include "rtc.h"
#include "ui.h"



extern Serial serial;



bool sd_ok = true;
bool gLive = true;

enum FILEACTIONS
{
	FA_RM = 1,
	FA_LS,
	FA_CAT,
	FA_DISK,
	FA_WRITE,
	FA_FIND,
};




// initialisation. Sets up the SPI controller if there is a card in the slot.
void
log_init (void)
{
	if (!sd_raw_available ())
	{
		LOG_INFO ("No SD Card plugged in\n");
		return;						  // no SD card plugged in!!
	}
	// Open SPI communication channel
	sd_ok = sd_raw_init ();

	if (!sd_ok)
	{
		LOG_WARN ("Unable to initialise SD card\n");
	}

}

// formats the data from a binary log record struct into a printable buffer
static void
format_record (uint8_t event, char *buffer)
{
	float power;
	uint8_t flags = event, month, day;

	get_month_day(&month, &day);

	if (gLoad != LOADOFF)
		flags |= LOG_LOAD;
	if (gDump > 0)
		flags |= LOG_SHUNT;

	sprintf (buffer, "%02d-%02d-%02d %02d:%02d:%02d ", day, month, gYEAR, gHOUR, gMINUTE, gSECOND);
	sprintf (buffer + strlen (buffer), "E:%c L:%c S:%c F:%d ", flags & LOG_ERROR ? '1' : '0', flags & LOG_LOAD ? '1' : '0', flags & LOG_SHUNT ? '1' : '0', flags & LOG_MASK_VALUE);

	sprintf (buffer + strlen (buffer), "D:%d T:%.*s%d.%02u C:%u ", gDump, gTemp < 0 ? 1 : 0, "-", abs (gTemp / 100), abs (gTemp % 100), gCharge);
	sprintf (buffer + strlen (buffer), "V:%d.%02u A:%.*s%d.%02u ", gVolts / 100, gVolts % 100, gAmps < 0 ? 1 : 0, "-", abs (gAmps / 100), abs (gAmps % 100));

	// volts & amps are scaled by 100 each so loose 10,000
	power = ((float) gAmps * (float) gVolts) / 10000.0;
	sprintf (buffer + strlen (buffer), "P:%d R:%d r:%d H:%d Y:%d h:%d y:%d I:%u O:%u\r\n", (int16_t) power, gRPM, gMaxRPM, gMaxhour, gMaxday, gMinhour, gMinday, (uint16_t)gCCA, (uint16_t)gDCA);
	return;
}

// find a file in a directory by name
static uint8_t
find_file_in_dir (struct fat_dir_struct *dd, const char *name, struct fat_dir_entry_struct *dir_entry)
{
	while (fat_read_dir (dd, dir_entry))
	{
		if (strcmp (dir_entry->long_name, name) == 0)
		{
			fat_reset_dir (dd);
			return 1;
		}
	}

	return 0;
}

// open a file by name
static struct fat_file_struct *
open_file_in_dir (struct fat_fs_struct *fs, struct fat_dir_struct *dd, const char *name)
{
	struct fat_dir_entry_struct file_entry;
	if (!find_file_in_dir (dd, name, &file_entry))
		return 0;

	return fat_open_file (fs, &file_entry);
}

// all you wanted to know about the SD card
static uint8_t
print_disk_info (const struct fat_fs_struct *fs)
{
	if (!fs)
		return 0;

	struct sd_raw_info disk_info;
	if (!sd_raw_get_info (&disk_info))
		return 0;

	kfile_printf (&serial.fd, "manuf:  %x\r\n", disk_info.manufacturer);
	kfile_printf (&serial.fd, "oem:    %s\r\n", disk_info.oem);
	kfile_printf (&serial.fd, "prod:   %s\r\n", disk_info.product);
	kfile_printf (&serial.fd, "rev:    %x\r\n", disk_info.revision);
	kfile_printf (&serial.fd, "serial: %lx\r\n", disk_info.serial);
	kfile_printf (&serial.fd, "date:   %d/%d\r\n", disk_info.manufacturing_month, disk_info.manufacturing_year);
	kfile_printf (&serial.fd, "size:   %dMB\r\n", (int)disk_info.capacity / 1024 / 1024);
	kfile_printf (&serial.fd, "copy:   %d\r\n", disk_info.flag_copy);
	kfile_printf (&serial.fd, "wr.pr.: %d/%d\r\n", disk_info.flag_write_protect_temp, disk_info.flag_write_protect);
	kfile_printf (&serial.fd, "format: %d\r\n", disk_info.format);
	kfile_printf (&serial.fd, "free:   %lu/%lu\r\n", (long unsigned int)fat_get_fs_free (fs), (long unsigned int)fat_get_fs_size (fs));

	return 1;
}

// take one of several actions on a file or the whole filesystem. Done in one lump as there is a lot of common
// code to open the partition and root directory for all actions.
static void
file_action (char *filename, enum FILEACTIONS action, char *data)
{

	if ((!sd_ok) || (!sd_raw_available ()))
	{
		sd_ok = false;				  // if card removed, force a re-init
		return;
	}

	/* open first partition */
	struct partition_struct *partition = partition_open (sd_raw_read, sd_raw_read_interval, sd_raw_write, sd_raw_write_interval, 0);

	if (!partition)
	{
		/* If the partition did not open, assume the storage device
		 * is a "superfloppy", i.e. has no MBR.
		 */
		partition = partition_open (sd_raw_read, sd_raw_read_interval, sd_raw_write, sd_raw_write_interval, -1);
		if (!partition)
		{
			LOG_WARN ("opening partition failed\n");
			sd_ok = false;
			return;
		}
	}

	/* open file system */
	struct fat_fs_struct *fs = fat_open (partition);
	if (!fs)
	{
		LOG_WARN ("opening filesystem failed\n");
		partition_close (partition);
		sd_ok = false;
		return;
	}

	/* open root directory */
	struct fat_dir_entry_struct directory;
	fat_get_dir_entry_of_path (fs, "/", &directory);

	struct fat_dir_struct *dd = fat_open_dir (fs, &directory);
	if (!dd)
	{
		LOG_WARN ("opening root directory failed\n");
		fat_close (fs);
		partition_close (partition);
		sd_ok = false;
		return;
	}

	switch (action)
	{
	case FA_WRITE:
		{
			/* search file in current directory and open it */
			struct fat_file_struct *fd = open_file_in_dir (fs, dd, filename);
			if (!fd)
			{
				LOG_INFO ("error opening %s - creating\n", filename);
				struct fat_dir_entry_struct file_entry;
				if (!fat_create_file (dd, filename, &file_entry))
				{
					LOG_WARN ("error creating file: %s\n", filename);
					return;
				}
				fd = open_file_in_dir (fs, dd, filename);
			}
// seek to eof

			/* search file in current directory and open it */
			if (!fd)
			{
				LOG_WARN ("error opening %s\n", filename);
				return;
			}

			int32_t offset = 0;
			if (!fat_seek_file (fd, &offset, FAT_SEEK_END))
			{
				LOG_WARN ("error seeking on %s\n", filename);
				fat_close_file (fd);
				return;
			}

			uint8_t data_len = strlen (data);
			/* write text to file */
			if (fat_write_file (fd, (uint8_t *) data, data_len) != data_len)
			{
				LOG_WARN ("error writing to file %s\n", filename);
				return;
			}
			fat_close_file (fd);
			if (!sd_raw_sync ())
				LOG_WARN ("error syncing disk\n");

			break;
		}
	case FA_RM:
		{
			if (strlen (filename) == 0)
				return;

			struct fat_dir_entry_struct file_entry;
			if (find_file_in_dir (dd, filename, &file_entry))
			{
				if (fat_delete_file (fs, &file_entry))
					return;
			}

			kfile_printf (&serial.fd, "error deleting file: %s\r\n", filename);
			break;
		}
	case FA_LS:
		{
			/* print directory listing */
			struct fat_dir_entry_struct dir_entry;
			while (fat_read_dir (dd, &dir_entry))
			{
				kfile_printf (&serial.fd, "%s%c  %10lu\r\n", dir_entry.long_name,
								  dir_entry.attributes & FAT_ATTRIB_DIR ? '/' : ' ', dir_entry.file_size);
			}
			break;
		}

	case FA_CAT:
	case FA_FIND:
		{
			uint8_t buffer[140];
			int16_t len;
			char * p;
			int32_t offset;

			if (strlen (filename) == 0)
				return;

			/* search file in current directory and open it */
			struct fat_file_struct *fd = open_file_in_dir (fs, dd, filename);
			if (!fd)
			{
				kfile_printf (&serial.fd, "error opening %s\r\n", filename);
				return;
			}

			while ((len = fat_read_file (fd, buffer, sizeof (buffer) - 1)) > 0)
			{
				if ((kfile_getc (&serial.fd) & 0x7f) == 0x1b)
					break;

				buffer[len] = 0;
				// terminate string at a new line, remember where the newline was
				if ((p = strchr((const char *)buffer, '\n')))
					*p++ = 0;
				// if doing a find then see if the data is present in this line
				if (data != NULL)
				{
					if (strstr((const char *)buffer, data))
						kfile_printf (&serial.fd, "%s\n", buffer);
				}
				else
					kfile_printf (&serial.fd, "%s\n", buffer);

				// see how much of the buffer we didn't use and seek back that far to get the start of the next line
				offset = (p - (char *)buffer) - len;

				if(!fat_seek_file(fd, &offset, FAT_SEEK_CUR))
					break;

			}
			fat_close_file (fd);
			break;
		}
	case FA_DISK:
		{
			/* print some card information */
			print_disk_info (fs);

			break;
		}
	}
	/* close directory */
	fat_close_dir (dd);

	/* close file system */
	fat_close (fs);

	/* close partition */
	partition_close (partition);

}


// store a record in the sd card filesystem in printable form
static void
log_store (uint8_t event)
{
	char filename[20];
	char print_buffer[140];

	// make a new log file name each day - now allows long filenames!!
	sprintf (filename, "log-%02d%02d%02d.txt", gYEAR, gMONTH, gDAY);

	format_record (event, print_buffer);

	file_action (filename, FA_WRITE, print_buffer);

}


// output a record in printable form to the uart
static void
log_print (uint8_t event)
{
	char print_buffer[140];

	format_record (event, print_buffer);
	kfile_printf (&serial.fd, "%s", print_buffer);

}

static char *
get_decimal (char *ptr, uint16_t * v)
{
	int16_t t = 0;

	while (*ptr >= '0' && *ptr <= '9')
		t = (t * 10) + (*ptr++ - '0');

	// skip over terminator
	ptr++;

	*v = t;
	return ptr;
}


// interpret a command from the serial interface
static void
process_command (char *command, uint8_t count)
{

	if (count == 0)				  // display now (empty input!!)
	{
		log_print (LOG_NULL);
	}

	else if (strncmp (command, "init", 4) == 0)	// first time init
	{
		do_first_init();
	}

	else if (strncmp (command, "cal", 3) == 0)	// calibrate (especially current)
	{
		lcd_bl_off();
		do_calibration ();
	}

	else if (strncmp (command, "sync", 4) == 0)	// set the charge value now
	{
		uint16_t c = gCharge;

		command += 4;
		if (command[0] != '\0')
		{
			while (*command == ' ')          // skip spaces
				command++;
			command = get_decimal (command, &c);
		}
		if (((int16_t)c > (gBankSize / 10)) && ((int16_t)c < (gBankSize * 2)))       // between 1/10th and double bank size
		{
			set_charge (c);
			kfile_printf (&serial.fd, "Charge synced to %d\r\n", c);
		}
		else
		{
			kfile_printf (&serial.fd, "Bad sync value %d\r\n", c);
		}
	}

	else if (strncmp (command, "dcs", 3) == 0)	// set the ratio of the DCS and CCS registers as a % value (0-99)
	{
		uint16_t p = 0, b = 0;

		command += 3;
		if (command[0] != '\0')
		{
			while (*command == ' ')
				command++;
			command = get_decimal (command, &p);
			command = get_decimal (command, &b);
		}
		kfile_printf (&serial.fd, "DCS/CCS set\r\n");
		do_CCADCA (p, b);
	}

	else if (strncmp (command, "inv", 3) == 0)
	{
		kfile_printf (&serial.fd, "Inverter ");
		if (gLoad == LOADOFF)
		{
			do_command (MANUALON);
			kfile_printf (&serial.fd, "ON\r\n");
		}
		else
		{
			do_command (MANUALOFF);
			kfile_printf (&serial.fd, "OFF\r\n");
		}
	}

	else if (strncmp (command, "start", 5) == 0)
	{
		kfile_printf (&serial.fd, "Starting\r\n");
		do_command (MANUALSTART);
	}

	else if (strncmp (command, "stop", 4) == 0)
	{
		kfile_printf (&serial.fd, "Stopping\r\n");
		do_command (MANUALSTOP);
	}

	else if (strncmp (command, "log", 3) == 0)
	{
		gLive = !gLive;
		kfile_printf (&serial.fd, "Logging %s\r\n", gLive ? "ON" : "OFF");
	}

	else if (strncmp (command, "date", 4) == 0)
	{
		uint16_t t;
		bool res = false;
		uint8_t month, day;

		get_month_day(&month, &day);

		command += 4;
		while (*command == ' ')
			command++;

		if (command[0] == '\0')
		{
			kfile_printf (&serial.fd, "%02d-%02d-%02d\r\n", day, month, gYEAR);
		}
		else
		{
			// parse date
			command = get_decimal (command, &t);
			res |= check_value (eDAY, t);
			command = get_decimal (command, &t);
			res |= check_value (eMONTH, t);
			command = get_decimal (command, &t);
			res |= check_value (eYEAR, t);
			if (res)
				kfile_printf (&serial.fd, "Invalid date\r\n");
			else
				set_epoch_time ();
		}
	}

	else if (strncmp (command, "time", 4) == 0)
	{
		uint16_t t;
		bool res = false;

		// skip command string
		command += 4;
		// skip whitespace
		while (*command == ' ')
			command++;
		if (command[0] == '\0')
		{
			kfile_printf (&serial.fd, "%02d:%02d:%02d\r\n", gHOUR, gMINUTE, gSECOND);
		}
		else
		{
			// parse time
			command = get_decimal (command, &t);
			res |= check_value (eHOUR, t);
			command = get_decimal (command, &t);
			res |= check_value (eMINUTE, t);
			command = get_decimal (command, &t);
			res |= check_value (eSECOND, t);
			if (res)
				kfile_printf (&serial.fd, "Invalid time\r\n");
			else
				set_epoch_time ();
		}
	}

	else if (strncmp (command, "uptime", 6) == 0)
	{
		uint32_t t = uptime();
		uint32_t d, h, m, s;
		d = t / 86400;
		h = (t - (d * 86400)) / 3600;
		m = (t - (d * 86400) - (h * 3600)) / 60;
		s = t % 60;
		// convert start of day to something readable
		kfile_printf (&serial.fd, "%lu %02lu:%0lu:%02lu\r\n", d, h, m, s);
	}

	else if (strncmp (command, "dir", 3) == 0)
	{
		file_action (NULL, FA_LS, NULL);
	}

	else if (strncmp (command, "disk", 4) == 0)
	{
		file_action (NULL, FA_DISK, NULL);
	}

	// cat <filename>
	else if (strncmp (command, "type ", 5) == 0)
	{
		command += 5;
		while (*command == ' ')
			command++;
		file_action (command, FA_CAT, NULL);
	}

	// rm <filename>
	else if (strncmp (command, "del ", 4) == 0)
	{
		command += 4;
		while (*command == ' ')
			command++;
		if (command[0] == '\0')
			return;
		file_action (command, FA_RM, NULL);
	}

	// find [-<days>] <string>
	else if (strncmp (command, "find ", 5) == 0)
	{
		char filename[20];
		uint16_t days = 0;

		command += 5;
		while (*command == ' ')
			command++;                   // skip spaces
		// see if the number of days is defined
		if (*command == '-')
		{
			command++;
			command = get_decimal (command, &days);
			days--;
			while (*command == ' ')
				command++;                   // skip spaces
		}

		if (command[0] == '\0')
			return;

		if (gDAY - days <= 0)
		{
			kfile_printf(&serial.fd, "Can't `find` back to last month!!\r\n");
			return;
		}
		while ((int16_t)days >= 0)
		{
			sprintf (filename, "log-%02d%02d%02d.txt", gYEAR, gMONTH, gDAY - days);
			kfile_printf(&serial.fd, "Checking %s for %s\r\n", filename, command);
			file_action (filename, FA_FIND, command);
			days--;
		}
	}

	else if (strncmp (command, "config", 6) == 0)
	{
		char tritext[4][5] = {"off", "on", "auto", "" };

		kfile_printf(&serial.fd, "System %dV, inverter %s\r\n", gVoltage, tritext[gInverter]); 
		kfile_printf(&serial.fd, "Low - High limits   %d.%02u - %d.%02u\r\n", gVlower / 100, gVlower % 100, gVupper / 100, gVupper % 100);
		kfile_printf(&serial.fd, "Float - Absorb      %d.%02u - %d.%02u\r\n", gFloatVolts / 100, gFloatVolts % 100, gAbsorbVolts / 100, gAbsorbVolts % 100);
		kfile_printf(&serial.fd, "Min/Max Charge - Bank    %d/%d - %d\r\n", gMinCharge, gMaxCharge,  gBankSize);
		kfile_printf(&serial.fd, "Self Discharge - Leak    %d  - %d.%02u\r\n", gSelfDischarge, gIdleCurrent / 100, gIdleCurrent % 100);
		kfile_printf(&serial.fd, "Float Cycle - Target     %d/%d - %d\r\n", gDischarge, gMaxDischarge, TargetC);
#if DEBUG > 0
extern int16_t gLoops;
		kfile_printf(&serial.fd, "Loop                     %d\r\n", gLoops);
extern uint16_t StackCount(void);
		kfile_printf(&serial.fd, "Stack free       %d\r\n", StackCount());
#endif
	}

	else
	{
		kfile_printf (&serial.fd, "Version " VERSION "\r\nCommands: ");
		kfile_printf (&serial.fd, "cal del dir type disk dcs init inv log date time find config sync uptime\r\n");
	}

//	kfile_printf(&serial.fd, ">> ");

}


// regular slices of CPU to get serial characters into a command buffer, output a regular log record and retry the sd card if its missing.
void
run_log (void)
{
	static int16_t LastTimerstamp = 0xff;
	int16_t c;
	static uint8_t bcnt = 0;
#define CBSIZE 20
	static char cbuff[CBSIZE];	  /* console I/O buffer       */

// on minute interval store a record with a timestamp
	if (LastTimerstamp != gMINUTE)	// use gSECOND for more frequent updates!!
	{
		if (!sd_ok)
			log_init ();
		LastTimerstamp = gMINUTE;
		log_event (LOG_MARKTIME);
	}

// poll uart RX and act on received character
#if PUSHBUTTONS != 1
	return;		// if no pushbuttons then I'm using the uart input for LCD menu
#endif

	while ((c = kfile_getc (&serial.fd)) != EOF)
	{
		c &= 0x7f;
		switch ((char) c)
		{
		case 0x03:					  // ctl-c
			bcnt = 0;
		case '\r':
// process what is in the buffer
			cbuff[bcnt] = '\0';	  // don't include terminator in count
			kfile_printf (&serial.fd, "\r\n\n");
			process_command (cbuff, bcnt);
			bcnt = 0;
			break;
		case 0x08:					  // backspace
		case 0x7f:					  // rubout
			if (bcnt > 0)
			{
				kfile_putc (0x08, &serial.fd);
				kfile_putc (' ', &serial.fd);
				kfile_putc (0x08, &serial.fd);
				bcnt--;
			}
			break;
		default:
			if ((c >= ' ') && (bcnt < CBSIZE))
			{
				cbuff[bcnt++] = c;
				kfile_putc (c, &serial.fd);	// echo...
			}
			break;
		}

	}

}


// store a record with timestamp and flags according to input parameter
void
log_event (uint8_t event)
{

	log_store (event);

	if (gLive)
		log_print (event);
}
