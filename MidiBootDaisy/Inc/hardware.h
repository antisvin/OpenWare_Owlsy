#include "main.h"
#include "stm32h7xx_hal.h"

#define USE_USBD_MIDI
#define USBD_MAX_POWER              100 // 100mA for iPad compatibility
#define USBD_HANDLE hUsbDeviceFS

#if defined DAISY
  #define HARDWARE_VERSION    "Daisy Boot"
  #define APPLICATION_ADDRESS 0x90000000
#else
  #error Invalid configuration
#endif

#define QSPI_HANDLE hqspi
//#define QSPI_DEVICE_IS25LP080D
#define QSPI_DEVICE_IS25LP064A
#define QSPI_FLASH_

/* #define USE_IWDG */
/* #define INIT_FMC */