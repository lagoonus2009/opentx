/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#if defined(__cplusplus)
extern "C" {
#endif
#include "usb_dcd_int.h"
#include "usb_bsp.h"
#if defined(__cplusplus)
}
#endif

#include "opentx.h"
#include "debug.h"

static bool usbDriverStarted = false;
#if defined(BOOT)
static usbMode selectedUsbMode = USB_MASS_STORAGE_MODE;
#else
static usbMode selectedUsbMode = USB_UNSELECTED_MODE;
#endif

int getSelectedUsbMode()
{
  return selectedUsbMode;
}

void setSelectedUsbMode(int mode)
{
  selectedUsbMode = usbMode (mode);
}

int usbPlugged()
{
  // debounce
  static uint8_t debounced_state = 0;
  static uint8_t last_state = 0;

#if defined(PCBI6X) && !defined(PCBI6X_USB_VBUS)
  if(globalData.usbDetect == USB_DETECT_ON) {
    return 1;
  }
#endif

  if (GPIO_ReadInputDataBit(USB_GPIO, USB_GPIO_PIN_VBUS)) {
    if (last_state) {
      debounced_state = 1;
    }
    last_state = 1;
  }
  else {
    if (!last_state) {
      debounced_state = 0;
    }
    last_state = 0;
  }
  return debounced_state;
}

#if defined(STM32F0)
USB_CORE_HANDLE USB_Device_dev;
#else
USB_OTG_CORE_HANDLE USB_OTG_dev;
#endif

#if defined(STM32F0)
extern "C" void USB_IRQHandler()
{
  USB_Istr();
}
#else
extern "C" void OTG_FS_IRQHandler()
{
  DEBUG_INTERRUPT(INT_OTG_FS);
  USBD_OTG_ISR_Handler(&USB_OTG_dev);
}
#endif

void usbInit()
{
  // Initialize hardware
#if defined(STM32F0)
  USB_BSP_Init(&USB_Device_dev);
#else
  USB_OTG_BSP_Init(&USB_OTG_dev);
#endif
  usbDriverStarted = false;
}

void usbStart()
{
  switch (getSelectedUsbMode()) {
#if !defined(BOOT)
    case USB_JOYSTICK_MODE:
      // initialize USB as HID device
#if defined(STM32F0)
      USBD_Init(&USB_Device_dev, &USR_desc, &USBD_HID_cb, &USR_cb);
#else
      USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_HID_cb, &USR_cb);
#endif
      break;
#endif
#if defined(USB_SERIAL) && !defined(PCBI6X)
    case USB_SERIAL_MODE:
      // initialize USB as CDC device (virtual serial port)
#if defined(STM32F0)
      USBD_Init(&USB_Device_dev, &USR_desc, &USBD_CDC_cb, &USR_cb);
#else
      USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
#endif
      break;
#endif
#if !defined(PCBI6X) || defined(PCBI6X_USB_MSD)
    default:
    case USB_MASS_STORAGE_MODE:
      // initialize USB as MSC device
#if defined(STM32F0)
      USBD_Init(&USB_Device_dev, &USR_desc, &USBD_MSC_cb, &USR_cb);
#else
      USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_MSC_cb, &USR_cb);
#endif
      break;
#endif
  }
  usbDriverStarted = true;
}

void usbStop()
{
  usbDriverStarted = false;
#if defined(STM32F0)
  USBD_DeInit(&USB_Device_dev);
#else
  USBD_DeInit(&USB_OTG_dev);
#endif
}

bool usbStarted()
{
  return usbDriverStarted;
}

#if !defined(BOOT)
/*
  Prepare and send new USB data packet

  The format of HID_Buffer is defined by
  USB endpoint description can be found in
  file usb_hid_joystick.c, variable HID_JOYSTICK_ReportDesc
*/
void usbJoystickUpdate()
{
  static uint8_t HID_Buffer[HID_IN_PACKET];

  // test to see if TX buffer is free
#if defined(STM32F0)
  if (USBD_HID_SendReport(&USB_Device_dev, 0, 0) == USBD_OK) {
#else
  if (USBD_HID_SendReport(&USB_OTG_dev, 0, 0) == USBD_OK) {
#endif
    //buttons
    HID_Buffer[0] = 0;
    HID_Buffer[1] = 0;
    HID_Buffer[2] = 0;
    for (int i = 0; i < 8; ++i) {
      if ( channelOutputs[i+8] > 0 ) {
        HID_Buffer[0] |= (1 << i);
      }
      if ( MAX_OUTPUT_CHANNELS>=24 && channelOutputs[i+16] > 0 ) {
        HID_Buffer[1] |= (1 << i);
      }
      if ( MAX_OUTPUT_CHANNELS>=32 && channelOutputs[i+24] > 0 ) {
        HID_Buffer[2] |= (1 << i);
      }
    }

    //analog values
    //uint8_t * p = HID_Buffer + 1;
    for (int i = 0; i < 8; ++i) {

      int16_t value = channelOutputs[i] + 1024;
      if ( value > 2047 ) value = 2047;
      else if ( value < 0 ) value = 0;
#if defined(PCBI6X)
      HID_Buffer[i*2 +2] = static_cast<uint8_t>(value & 0xFF);
      HID_Buffer[i*2 +3] = static_cast<uint8_t>((value >> 8) & 0x07);
#else
      HID_Buffer[i*2 +3] = static_cast<uint8_t>(value & 0xFF);
      HID_Buffer[i*2 +4] = static_cast<uint8_t>((value >> 8) & 0x07);
#endif

    }

#if defined(PCBI6X)
    // HID_Buffer index 8 & 9 causes mess. Looks like clock issue but cannot confirm.
    // i reduced buttons to 16 so it will affect only one analog and remapped it [3] -> [5]
    HID_Buffer[12] = HID_Buffer[8]; // ch[3] remap to ch[5]  // channel 5 void
    HID_Buffer[13] = HID_Buffer[9];
    HID_Buffer[8] = 0;
    HID_Buffer[9] = 0;
#endif

#if defined(STM32F0)
    USBD_HID_SendReport(&USB_Device_dev, HID_Buffer, HID_IN_PACKET);
#else
    USBD_HID_SendReport(&USB_OTG_dev, HID_Buffer, HID_IN_PACKET);
#endif
  }
}
#endif // BOOT
