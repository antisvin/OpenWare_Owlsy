#!/bin/bash
git checkout Inc/usbh_conf.h
git checkout Src/usb_device.c Src/usb_host.c Makefile
git checkout Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_hcd.c
rm -f Src/usbd_audio_if.c Inc/usbd_audio_if.h
