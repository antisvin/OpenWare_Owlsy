#include "main.h"
#include "stm32h7xx_hal.h"
#include "hardware_ids.h"

#define USE_USBD_MIDI
#define USBD_HANDLE hUsbDeviceFS
#define USE_BOOTLOADER_MODE


#define MAX_SYSEX_FIRMWARE_SIZE 512 * 1024

#if defined DAISY_PATCH
  #define HARDWARE_ID         DAISY_PATCH_HARDWARE
  #define HARDWARE_VERSION    "Daisy Boot"
  #define APPLICATION_ADDRESS 0x90000000
  #define USE_BOOT1_PIN
  #define BOOT1_Pin GPIO_PIN_12
  #define BOOT1_GPIO_Port GPIOB  
#elif defined BLUEMCHEN
  #define HARDWARE_ID         BLUEMCHEN_HARDWARE
  #define HARDWARE_VERSION    "Daisy Boot"
  #define APPLICATION_ADDRESS 0x90000000
  #define USE_BOOT1_PIN
  #define BOOT1_Pin GPIO_PIN_2
  #define BOOT1_GPIO_Port GPIOA
#elif defined DAISY_SEED
  #define HARDWARE_ID         DAISY_SEED_HARDWARE
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

#define DAISY_APPLICATION_ADDRESS 0x90040000
#define DAISY_STACK_END           0x20020000

#define USE_IWDG

