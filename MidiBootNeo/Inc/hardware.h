#include "main.h"
#include "stm32l4xx_hal.h"

#define USE_USBD_MIDI
#define USBD_MAX_POWER              100 // 100mA for iPad compatibility
#define USBD_HANDLE hUsbDeviceFS
#define USE_USBD_FS
#define NO_EXTERNAL_RAM

// Page numbers for L4 flash
#define PAGE_SIZE                   8 * 1024
#define FIRMWARE_START_SECTOR       4
#define STORAGE_START_SECTOR        32
#define STORAGE_END_SECTOR          128

#define MAX_SYSEX_FIRMWARE_SIZE 256 * 1024

#define HARDWARE_VERSION    "Neophyte boot"
#define APPLICATION_ADDRESS 0x08008000

/* Firmware header configuration */
#define FIRMWARE_HEADER        0xBABECAFE

#define USE_IWDG

#if 0
#define USE_ENCODER_PIN
#define ENC_CLICK_Pin GPIO_PIN_12
#define ENC_CLICK_GPIO_Port GPIOB
#endif
