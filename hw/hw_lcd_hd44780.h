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
 * Copyright 2003, 2004, 2005, 2008 Develer S.r.l. (http://www.develer.com/)
 * Copyright 2001 Bernie Innocenti <bernie@codewiz.org>
 *
 * -->
 *
 * \brief LCD low-level hardware macros
 *
 * \author Bernie Innocenti <bernie@codewiz.org>
 * \author Stefano Fedrigo <aleph@develer.com>
 *
 */

#ifndef HW_LCD_HD44780_H
#define HW_LCD_HD44780_H

#include "cfg/cfg_lcd_hd44780.h"  /* CONFIG_LCD_4BIT & CONFIG_LCD_I2C */

#include <cpu/types.h>
#include <cpu/irq.h>



#include <drv/pcf8574.h>
#include <cfg/macros.h>

#define PCF8574_DEVICEID  0 //device id, addr = pcf8574 base addr + PCF8574_DEVICEID



#define LCD_DATA0_PIN    4            /**< pin for 4bit data bit 0     */
#define LCD_DATA1_PIN    5            /**< pin for 4bit data bit 1     */
#define LCD_DATA2_PIN    6            /**< pin for 4bit data bit 2     */
#define LCD_DATA3_PIN    7            /**< pin for 4bit data bit 3     */
#define LCD_RS_PIN       0            /**< pin  for RS line            */
#define LCD_RW_PIN       1            /**< pin  for RW line            */
#define LCD_E_PIN        2            /**< pin  for Enable line        */
#define LCD_LED_PIN      3            /**< pin  for Led                */
#define LCD_LED_POL      1            /**< polarity of Led pin (1=+ve) */


volatile uint8_t dataport = 0;

/*
** local functions
*/



/* toggle Enable Pin to initiate write */
static void
lcd_e_toggle (void)
{
   pcf8574_setoutputpinhigh (PCF8574_DEVICEID, LCD_E_PIN);
   timer_udelay (1);;
   pcf8574_setoutputpinlow (PCF8574_DEVICEID, LCD_E_PIN);
}


/*************************************************************************
Low-level function to write byte to LCD controller
Input:    data   byte to write to LCD
          rs     1: write data    
                 0: write instruction
Returns:  none
*************************************************************************/
static void
lcd_write (uint8_t data, uint8_t rs)
{
   if (rs)                      /* write data        (RS=1, RW=0) */
      dataport |= BV (LCD_RS_PIN);
   else                         /* write instruction (RS=0, RW=0) */
      dataport &= ~BV (LCD_RS_PIN);
   dataport &= ~BV (LCD_RW_PIN);
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);

   /* output high nibble first */
   dataport &= ~BV (LCD_DATA3_PIN);
   dataport &= ~BV (LCD_DATA2_PIN);
   dataport &= ~BV (LCD_DATA1_PIN);
   dataport &= ~BV (LCD_DATA0_PIN);
   if (data & 0x80)
      dataport |= BV (LCD_DATA3_PIN);
   if (data & 0x40)
      dataport |= BV (LCD_DATA2_PIN);
   if (data & 0x20)
      dataport |= BV (LCD_DATA1_PIN);
   if (data & 0x10)
      dataport |= BV (LCD_DATA0_PIN);
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);
   lcd_e_toggle ();

   /* output low nibble */
   dataport &= ~BV (LCD_DATA3_PIN);
   dataport &= ~BV (LCD_DATA2_PIN);
   dataport &= ~BV (LCD_DATA1_PIN);
   dataport &= ~BV (LCD_DATA0_PIN);
   if (data & 0x08)
      dataport |= BV (LCD_DATA3_PIN);
   if (data & 0x04)
      dataport |= BV (LCD_DATA2_PIN);
   if (data & 0x02)
      dataport |= BV (LCD_DATA1_PIN);
   if (data & 0x01)
      dataport |= BV (LCD_DATA0_PIN);
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);
   lcd_e_toggle ();

   /* all data pins high (inactive) */
   dataport |= BV (LCD_DATA0_PIN);
   dataport |= BV (LCD_DATA1_PIN);
   dataport |= BV (LCD_DATA2_PIN);
   dataport |= BV (LCD_DATA3_PIN);
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);
}


/*************************************************************************
Low-level function to read byte from LCD controller
Input:    rs     1: read data    
                 0: read busy flag / address counter
Returns:  byte read from LCD controller
*************************************************************************/
static uint8_t
lcd_read (uint8_t rs)
{
   uint8_t data;

   if (rs)                      /* write data        (RS=1, RW=0) */
      dataport |= BV (LCD_RS_PIN);
   else                         /* write instruction (RS=0, RW=0) */
      dataport &= ~BV (LCD_RS_PIN);
   dataport |= BV (LCD_RW_PIN);
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);

   pcf8574_setoutputpinhigh (PCF8574_DEVICEID, LCD_E_PIN);
   timer_udelay (1);;
   data = pcf8574_getoutputpin (PCF8574_DEVICEID, LCD_DATA0_PIN) << 4; /* read high nibble first */
   pcf8574_setoutputpinlow (PCF8574_DEVICEID, LCD_E_PIN);

   timer_udelay (1);;              /* Enable 500ns low       */

   pcf8574_setoutputpinhigh (PCF8574_DEVICEID, LCD_E_PIN);
   timer_udelay (1);;
   data |= pcf8574_getoutputpin (PCF8574_DEVICEID, LCD_DATA0_PIN) & 0x0F; /* read low nibble        */
   pcf8574_setoutputpinlow (PCF8574_DEVICEID, LCD_E_PIN);

   return data;
}

/*************************************************************************
Set illumination pin
*************************************************************************/
void
lcd_backlight (uint8_t onoff)
{
   if ((onoff ^ LCD_LED_POL) & 1)
      dataport &= ~BV (LCD_LED_PIN);
   else
      dataport |= BV (LCD_LED_PIN);
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);
}


static void
lcd_hw_init (void)
{
   pcf8574_init ();

   dataport = 0;
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);

   timer_udelay (16000);               /* wait 16ms or more after power-on       */

   /* initial write to lcd is 8bit */
   dataport |= BV (LCD_DATA1_PIN); // BV(LCD_FUNCTION)>>4;
   dataport |= BV (LCD_DATA0_PIN); // BV(LCD_FUNCTION_8BIT)>>4;
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);

   lcd_e_toggle ();
   timer_udelay (4992);                /* delay, busy flag can't be checked here */

   /* repeat last command */
   lcd_e_toggle ();
   timer_udelay (64);                  /* delay, busy flag can't be checked here */

   /* repeat last command a third time */
   lcd_e_toggle ();
   timer_udelay (64);                  /* delay, busy flag can't be checked here */

   /* now configure for 4bit mode */
   dataport &= ~BV (LCD_DATA0_PIN);
   pcf8574_setoutput (PCF8574_DEVICEID, dataport);
   lcd_e_toggle ();
   timer_udelay (64);                  /* some displays need this additional delay */

#if 0
   /* from now the LCD only accepts 4 bit I/O, we can use lcd_command() */

   lcd_command(LCD_CMD_SETFUNC);      /* function set: display lines  */

   lcd_command (LCD_CMD_DISPLAY_OFF);  /* display off                  */
   lcd_clrscr ();               /* display clear                */
   lcd_command (LCD_CMD_DISPLAYMODE);  /* set entry mode               */
   lcd_command (LCD_CMD_DISPLAY_OFF | LCD_CMD_ON_DISPLAY);      /* display/cursor control       */
#endif

}                               /* lcd_init */


#endif /* HW_LCD_HD44780_H */
