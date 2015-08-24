//---------------------------------------------------------------------------
// Copyright (C) 2012 Robin Gilks
//
//
//  main.c   -   This program measures the output from and controls the load on a wind turbine
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

#include <drv/timer.h>
#include <drv/ser.h>
#include <drv/lcd_hd44.h>
#include <drv/term.h>
#include <drv/kbd.h>

#include "features.h"

#include "eeprommap.h"
#include "measure.h"
#include "control.h"
#include "tlog.h"
#include "rpm.h"
#include "rtc.h"
#include "graph.h"
#include "ui.h"
#include "nrf.h"

Serial serial;




static void init(void)
{

	/* Initialize debugging module (allow kprintf(), etc.) */
//	kdbg_init();
	/* Initialize system timer */
	timer_init();

	/*
	 * XXX: Arduino has a single UART port that was previously
	 * initialized for debugging purpose.
	 * In order to activate the serial driver you should disable 
	 * the debugging module.
	 */
	/* Initialize UART0 */
	ser_init(&serial, SER_UART0);
	/* Configure UART0 to work at 115.200 bps */
	ser_setbaudrate(&serial, 115200);

	/* Enable all the interrupts now the basic stuff is initialised */
	IRQ_ENABLE;

	// set clock up using last values from eeprom
	rtc_init();
	// read a few more values out of eeprom and init the display etc
	load_eeprom_values();
	// user interface
	ui_init();
	// voltage, current and temperature measurement
	measure_init();
	// dump load and inverter control
	control_init();
	// measure turbine speed
	rpm_init();
	// block graphics
	graph_init();
	// data reporting and serial command interface
	log_init();
   // initialise RF link to remote
   nrf_init();

}



int main(void)
{
   uint8_t key = 0;

	init();

	while (1)
	{
		// keep the clock ticking
		run_rtc();
      // run volts/amps/temperature reading stuff on the onewire interface
      run_measure();
      // decide if inverter or shunt load is needed to be turned on
      run_control();
      // keep the log data in dataflash up to date
      run_log();
      // calculate turbine RPM from period of raw AC
      run_rpm();
      // save values for graphic display of power in/out
      run_graph();
      // send data back to base
      key = run_nrf ();
      // display stuff on the LCD & get user input
      run_ui (key);

	}
}

#if DEBUG > 0

extern uint8_t _end;
extern uint8_t __stack; 

#define STACK_CANARY  0xc5

void StackPaint(void) __attribute__ ((naked)) __attribute__ ((section (".init1")));
uint16_t StackCount(void);

void StackPaint(void)
{
#if 1
    uint8_t *p = &_end;

    while(p <= &__stack)
    {
        *p = STACK_CANARY;
        p++;
    }
#else
    __asm volatile ("    ldi r30,lo8(_end)\n"
                    "    ldi r31,hi8(_end)\n"
                    "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xc5 */
                    "    ldi r25,hi8(__stack)\n"
                    "    rjmp .cmp\n"
                    ".loop:\n"
                    "    st Z+,r24\n"
                    ".cmp:\n"
                    "    cpi r30,lo8(__stack)\n"
                    "    cpc r31,r25\n"
                    "    brlo .loop\n"
                    "    breq .loop"::);
#endif
} 

uint16_t StackCount(void)
{
    const uint8_t *p = &_end;
    uint16_t       c = 0;

    while(*p == STACK_CANARY && p <= &__stack)
    {
        p++;
        c++;
    }

    return c;
} 

#endif
