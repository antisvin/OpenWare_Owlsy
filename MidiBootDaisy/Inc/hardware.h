#include "main.h"
#include "stm32h7xx_hal.h"

#define USE_USBD_MIDI
#define USBD_MAX_POWER              100 // 100mA for iPad compatibility
#define USBD_HANDLE hUsbDeviceFS

#define MAX_SYSEX_FIRMWARE_SIZE 512 * 1024

#if defined DAISY
  #define HARDWARE_VERSION    "Daisy Boot"
  #define APPLICATION_ADDRESS 0x90000000
#else
  #error Invalid configuration
#endif

#define QSPI_HANDLE hqspi
#define QSPI_DEVICE_IS25LP064A

/* Firmware header configuration */
#define FIRMWARE_HEADER        0xBABECAFE
#define FIRMWARE_RELOCATIONS_COUNT 5

#define USE_IWDG

#if 0
#define USE_ENCODER_PIN
#define ENC_CLICK_Pin GPIO_PIN_12
#define ENC_CLICK_GPIO_Port GPIOB
#endif
