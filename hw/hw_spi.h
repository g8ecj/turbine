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
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * Copyright 2008 Develer S.r.l. (http://www.develer.com/)
 * All Rights Reserved.
 * -->
 *
 * \brief Hardware macro definition.
 *
 * \author Daniele Basile <asterix@develer.com>
 */

#ifndef HW_SPI_H
#define HW_SPI_H

#include <avr/io.h>
#include "cfg/macros.h"   /* BV() */
/**
 * SPI pin definition.
 */
#define SS       PA0
#define SCK      PA1
#define MOSI     PA2
#define MISO     PA3
/*\}*/

#define MOSI_LOW()       do { PORTA &= ~BV(MOSI); } while(0)
#define MOSI_HIGH()      do { PORTA |= BV(MOSI);  } while(0)

#define SS_ACTIVE()      do { PORTA &= ~BV(SS); } while(0)
#define SS_INACTIVE()    do { PORTA |= BV(SS); } while(0)

#define SCK_INACTIVE()   do { PORTA &= ~BV(SCK); } while(0)
#define SCK_ACTIVE()     do { PORTA |= BV(SCK); } while(0)

#define IS_MISO_HIGH()   (PINA & BV(MISO))

#define SCK_PULSE()\
	do { \
			SCK_ACTIVE();\
			/* NOP; */ \
			SCK_INACTIVE();\
	} while(0)


#define SPI_HW_INIT() \
	do { \
		DDRA |= BV(MOSI) | BV(SS) | BV(SCK); \
		DDRA &= ~BV(MISO); \
		SS_INACTIVE(); \
		MOSI_LOW(); \
		SCK_INACTIVE(); \
	} while(0)

#endif /* HW_SPI_H */

