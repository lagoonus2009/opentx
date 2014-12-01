/*
 * Authors (alphabetical order)
 * - Andre Bernet <bernet.andre@gmail.com>
 * - Andreas Weitl
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 * - Cameron Weeks <th9xer@gmail.com>
 * - Erez Raviv
 * - Gabriel Birkus
 * - Jean-Pierre Parisy
 * - Karl Szmutny
 * - Michael Blandford
 * - Michal Hlavinka
 * - Pat Mackenzie
 * - Philip Moss
 * - Rob Thomson
 * - Romolo Manfredini <romolo.manfredini@gmail.com>
 * - Thomas Husterer
 *
 * opentx is based on code named
 * gruvin9x by Bryan J. Rentoul: http://code.google.com/p/gruvin9x/,
 * er9x by Erez Raviv: http://code.google.com/p/er9x/,
 * and the original (and ongoing) project by
 * Thomas Husterer, th9x: http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "../../opentx.h"

#if defined(__cplusplus) && !defined(SIMU)
extern "C" {
#endif
#include "STM32_USB-Host-Device_Lib_V2.1.0/Libraries/STM32_USB_OTG_Driver/inc/usb_dcd_int.h"
#include "STM32_USB-Host-Device_Lib_V2.1.0/Libraries/STM32_USB_OTG_Driver/inc/usb_bsp.h"
#include "STM32F2xx_StdPeriph_Lib_V1.1.0/Libraries/STM32F2xx_StdPeriph_Driver/inc/stm32f2xx_dbgmcu.h"
#if defined(__cplusplus) && !defined(SIMU)
}
#endif

void configure_pins( uint32_t pins, uint16_t config )
{
  uint32_t address ;
  GPIO_TypeDef *pgpio ;
  uint32_t thispin ;
  uint32_t pos ;

  address = ( config & PIN_PORT_MASK ) >> 8 ;
  address *= (GPIOB_BASE-GPIOA_BASE) ;
  address += GPIOA_BASE ;
  pgpio = (GPIO_TypeDef* ) address ;

  /* -------------------------Configure the port pins---------------- */
  /*-- GPIO Mode Configuration --*/
  for (thispin = 0x0001, pos = 0; thispin < 0x10000; thispin <<= 1, pos +=1 )
  {
    if ( pins & thispin)
    {
      pgpio->MODER  &= ~(GPIO_MODER_MODER0 << (pos * 2)) ;
      pgpio->MODER |= (config & PIN_MODE_MASK) << (pos * 2) ;

      if ( ( (config & PIN_MODE_MASK ) == PIN_OUTPUT) || ( (config & PIN_MODE_MASK) == PIN_PERIPHERAL) )
      {
        /* Speed mode configuration */
        pgpio->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR0 << (pos * 2)) ;
        pgpio->OSPEEDR |= ((config & PIN_SPEED_MASK) >> 13 ) << (pos * 2) ;

        /* Output mode configuration*/
        pgpio->OTYPER  &= ~((GPIO_OTYPER_OT_0) << ((uint16_t)pos)) ;
        if ( config & PIN_ODRAIN )
        {
          pgpio->OTYPER |= (GPIO_OTYPER_OT_0) << pos ;
        }
      }
      /* Pull-up Pull down resistor configuration*/
      pgpio->PUPDR &= ~(GPIO_PUPDR_PUPDR0 << ((uint16_t)pos * 2));
      pgpio->PUPDR |= ((config & PIN_PULL_MASK) >> 2) << (pos * 2) ;

      pgpio->AFR[pos >> 3] &= ~(0x000F << ((pos & 7)*4)) ;
      pgpio->AFR[pos >> 3] |= ((config & PIN_PERI_MASK) >> 4) << ((pos & 7)*4) ;
    }
  }
}
