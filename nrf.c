
#include <drv/ser.h>
#include <drv/timer.h>
#include <net/nrf24l01.h>
#include "ui.h"
#include "nrf.h"


extern Serial serial;

#define KEYSTROKE 'K'

uint8_t addrtx0[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP0;
uint8_t addrtx1[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP1;

#if NRF24L01_PRINTENABLE == 1
static void
debug_prints (const char * s)
{
   kfile_printf(&serial.fd, "%s", s);
}
#endif


void
nrf_init(void)
{
   /* init hardware pins */
   nrf24l01_init ();
#if NRF24L01_PRINTENABLE == 1
   nrf24l01_printinfo (debug_prints);
#endif
}


uint8_t
run_nrf (void)
{
   int8_t status = 1, row;
   uint8_t ret = 0, r, c;
   uint8_t buffer[NRF24L01_PAYLOAD];

   // always see if any remote key presses
   if (nrf24l01_readready (NULL))
   {
      //read buffer
      nrf24l01_read (buffer);
      // see if a keyboard command. If so return the keycode
      if (buffer[0] == KEYSTROKE)
         ret = buffer[1];
   }

   // throttle data transfer by only doing every 'n' ms, controlled by the UI
   if (!ui_refresh_check())
      return ret;

   // get a screenfull of data from the UI and send it
   while ((row = ui_termrowget(&buffer[2])) >= 0)
   {
      nrf24l01_settxaddr (addrtx1);
      buffer[0] = row + '0';
      buffer[1] = '0';
      status &= nrf24l01_write(buffer);
      buffer[22] = 0;
      timer_delay(10);
   }

   // get the current cursor address if it is on and send it
   if (ui_termcursorget(&r, &c))
   {
      nrf24l01_settxaddr (addrtx1);
      buffer[0] = 'A';
      buffer[1] = r;
      buffer[2] = c;
      status &= nrf24l01_write(buffer);
   }
   // otherwise send an indication the cursor is off
   else
   {
      nrf24l01_settxaddr (addrtx1);
      buffer[0] = 'C';
      buffer[1] = r;
      buffer[2] = c;
      status &= nrf24l01_write(buffer);
   }

   // debug report via serial interface
   if (status != 1)
   {
      kfile_printf (&serial.fd, "> Tx failed\r\n");

      /* Retranmission count indicates the tranmission quality */
      status = nrf24_retransmissionCount ();
      kfile_printf (&serial.fd, "> Retranmission count: %d\r\n", status);
   }

   // return to RX mode ready for more remote keys
   nrf24l01_setRX();

   return ret;
}

