#include "main.h"
#include "stm32h7xx_hal.h"

#define USBD_MAX_POWER              100 // 100mA for iPad compatibility
#define USBD_HANDLE hUsbDeviceFS

#if defined BLUEMCHEN
  #define HARDWARE_ID                  BLUEMCHEN_HARDWARE
  #define HARDWARE_VERSION             "Owlsy"
#else
  #error Invalid configuration
#endif

#define USE_FIRMWARE_HEADER
#define FLASH_TASK_STACK_SIZE        (1024/sizeof(portSTACK_TYPE))

//#define PROGRAM_TASK_STACK_SIZE      (6*1024/sizeof(portSTACK_TYPE))
//#define UTILITY_TASK_STACK_SIZE      (1024/sizeof(portSTACK_TYPE))
//#define MANAGER_TASK_STACK_SIZE      (2048/sizeof(portSTACK_TYPE))

#define MAIN_LOOP_SLEEP_MS          20

//#define MINIMAL_BUILD

// FW size limit would matter only if we use bootloader, otherwise linker scripts sets this limit
#define MAX_SYSEX_FIRMWARE_SIZE 512 * 1024
// Program size is limited by patch RAM section size
//#define MAX_SYSEX_PROGRAM_SIZE 448 * 1024
//#define MAX_SYSEX_PROGRAM_SIZE 256 * 1024

#define MAX_NUMBER_OF_PATCHES       60
#define MAX_NUMBER_OF_RESOURCES     60
#define STORAGE_MAX_BLOCKS          128

#define USE_SCREEN
#define SSD1309
//#define OLED_DMA // DMA doesn't work for data transfers for some reason, commands are OK
//#define OLED_UPSIDE_DOWN
#define OLED_I2C                    hi2c1
#define OLED_I2C_ADDRESS            0x3C

#define USE_ADC
#define ADC_PERIPH hadc1
#define ADC_A 0
#define ADC_B 1
#define ADC_C 2
#define ADC_D 3
#define NOF_ADC_VALUES               4
#define NOF_PARAMETERS               40
// Buttons are encoder short/long click, 2 x gate in, 1 x gate out
#define NOF_BUTTONS                  (4 + 2 + 1)
//#define USE_ENCODERS
#define NOF_ENCODERS                 2 // Second encoder is virtual - toggled by software mode
//#define ENCODER_TIM1 htim4

#define LOAD_INDICATOR_PARAMETER     PARAMETER_BD
// Parameters A-B are used by ADC, so we can't set values for them

#define USE_EXTERNAL_RAM
#define USE_CACHE

//#define USE_UART_MIDI_TX
#define USE_UART_MIDI_RX
#define UART_MIDI_HANDLE huart1
#define UART_MIDI_RX_BUFFER_SIZE 256

#define USE_USBD_MIDI
#define USE_USBD_AUDIO
#define USE_USBD_AUDIO_TX  // microphone
//#define USE_USBD_AUDIO_RX // speaker
/* USB audio settings */
#define AUDIO_BITS_PER_SAMPLE       16
#define AUDIO_BYTES_PER_SAMPLE      (AUDIO_BITS_PER_SAMPLE/8)
/* #define AUDIO_INT32_TO_SAMPLE(x)    (__REV16((x)>>8)) */
/* #define AUDIO_SAMPLE_TO_INT32(x)    ((int32_t)(__REV16(x))<<8) */
#define AUDIO_INT32_TO_SAMPLE(x)    ((x)>>8)
#define AUDIO_SAMPLE_TO_INT32(x)    ((int32_t)(x)<<8)


#define USE_CODEC
#define USE_AK4556
#define USE_USBD_FS
#define USBD_HANDLE hUsbDeviceFS
#define AUDIO_BITS_PER_SAMPLE       16
#define AUDIO_BYTES_PER_SAMPLE      (AUDIO_BITS_PER_SAMPLE/8)
#define AUDIO_CHANNELS              2

#define QSPI_HANDLE hqspi
//#define QSPI_DEVICE_IS25LP080D
#define QSPI_DEVICE_IS25LP064A

#define USE_IWDG
#define IWDG_PERIPH IWDG1
/* #define INIT_FMC */

#define USE_FATFS
#define FATFS_USE_DMA
