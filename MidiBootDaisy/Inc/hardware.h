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
#elif defined PATCH_INIT
  #define HARDWARE_ID         PATCH_INIT_HARDWARE
  #define HARDWARE_VERSION    "Init Boot"
  #define APPLICATION_ADDRESS 0x90000000
  #define USE_BOOT1_PIN
  #define BOOT1_Pin GPIO_PIN_8
  #define BOOT1_GPIO_Port GPIOB
  #define USER_LED_Pin GPIO_PIN_5
  #define USER_LED_GPIO_Port GPIOA
#elif defined DAISY_POD
  #define HARDWARE_ID         DAISY_POD_HARDWARE
  #define HARDWARE_VERSION    "Daisy Boot"
  #define APPLICATION_ADDRESS 0x90000000
  #define USE_BOOT1_PIN
  #define BOOT1_Pin GPIO_PIN_6
  #define BOOT1_GPIO_Port GPIOB
  #define USER_LED_Pin GPIO_PIN_7
  #define USER_LED_GPIO_Port GPIOA
  #define ERROR_LED_Pin GPIO_PIN_1
  #define ERROR_LED_GPIO_Port GPIOB
#else
  #error Invalid configuration
#endif

#define QSPI_HANDLE hqspi
#define QSPI_DEVICE_IS25LP064A

/* Firmware header configuration */
#define FIRMWARE_HEADER        0xBABECAFE
#define FIRMWARE_RELOCATIONS_COUNT 5

#define USE_IWDG

