/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Copyright (C) 2012 Robin Gilks, Peter Dannegger, Martin Thomas
 * -->
 *
 * \brief Driver for Dallas 1-wire devices
 *
 *
 * \author Peter Dannegger (danni(at)specs.de)
 * \author Martin Thomas (mthomas(at)rhrk.uni-kl.de)
 * \author Robin Gilks (g8ecj(at)gilks.org)
 *
 * $WIZ$ module_name = "hw_1wire"
 */

#ifndef HW_1WIRE_H_
#define HW_1WIRE_H_

#include "drv/ow_1wire.h"
#include "cfg/cfg_1wire.h"
#include "cfg/compiler.h"

#include <stdint.h>
#include <avr/io.h>

#include <drv/timer.h>
#include <cpu/irq.h>



// if all devices are hanging off one pin (or there is only one device)
#if OW_ONE_BUS == 1

#define OW_GET_IN()   ( OW_IN & (1<<OW_PIN))
#define OW_OUT_LOW()  ( OW_OUT &= (~(1 << OW_PIN)) )
#define OW_OUT_HIGH() ( OW_OUT |= (1 << OW_PIN) )
#define OW_DIR_IN()   ( OW_DDR &= (~(1 << OW_PIN )) )
#define OW_DIR_OUT()  ( OW_DDR |= (1 << OW_PIN) )

#define OW_CONF_DELAYOFFSET 0

#else

/*******************************************/
/* Hardware connection                     */
/*******************************************/

/* Define OW_ONE_BUS if only one 1-Wire-Bus is used
   in the application -> shorter code.
   If not defined make sure to call ow_set_bus() before using 
   a bus. Runtime bus-select increases code size by around 300 
   bytes so use OW_ONE_BUS if possible */
//#define UNUSED(arg) (arg) __attribute__ ((unused))


#if ( CPU_FREQ < 1843200 )
#warning | Experimental multi-bus-mode is not tested for
#warning | frequencies below 1,84MHz. Use OW_ONE_WIRE or
#warning | faster clock-source (i.e. internal 2MHz R/C-Osc.).
#endif
#define OW_CONF_CYCLESPERACCESS 13
#define OW_CONF_DELAYOFFSET ( (uint16_t)( ((OW_CONF_CYCLESPERACCESS) * 1000000L) / CPU_FREQ ) )

/* set bus-config with ow_set_bus() */
uint8_t OW_PIN_MASK;
volatile uint8_t *OW_IN;
volatile uint8_t *OW_OUT;
volatile uint8_t *OW_DDR;

#define OW_GET_IN()   ( *OW_IN & OW_PIN_MASK )
#define OW_OUT_LOW()  ( *OW_OUT &= (uint8_t) ~OW_PIN_MASK )
#define OW_OUT_HIGH() ( *OW_OUT |= (uint8_t)  OW_PIN_MASK )
#define OW_DIR_IN()   ( *OW_DDR &= (uint8_t) ~OW_PIN_MASK )
#define OW_DIR_OUT()  ( *OW_DDR |= (uint8_t)  OW_PIN_MASK )

/**
 * Set the port/data direction input pin dynamically
 *
 * \param in input port
 * \param out output port
 * \param ddr data direction register
 * \param pin I/O pin (bit number on port)
 *
 */
void ow_set_bus (volatile uint8_t * in, volatile uint8_t * out, volatile uint8_t * ddr, uint8_t pin)
{
	OW_DDR = ddr;
	OW_OUT = out;
	OW_IN = in;
	OW_PIN_MASK = (1 << pin);
	ow_reset ();
}

#endif



#endif
