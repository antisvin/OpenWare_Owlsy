#include "main.h"
#include "stm32h7xx_hal.h"

#define USBD_MAX_POWER              100 // 100mA for iPad compatibility
#define USBD_HANDLE hUsbDeviceFS

#if defined DAISY
  #define HARDWARE_ID                  DAISY_PATCH_HARDWARE
  #define HARDWARE_VERSION             "Daisy Patch"
  #define APPLICATION_ADDRESS 0x90000000
#else
  #error Invalid configuration
#endif

#define MINIMAL_BUILD

#define USE_SCREEN
#define SSD1309
//#define OLED_DMA
// #define OLED_SOFT_CS
#define OLED_SPI hspi1
/* #define OLED_IT */
/* #define OLED_BITBANG */

#define USE_ADC
#define ADC_PERIPH hadc1
#define ADC_A 0
#define ADC_B 1
#define ADC_C 2
#define ADC_D 3
#define NOF_ADC_VALUES               4
#define NOF_PARAMETERS               20
// Buttons are encoder short/long click, 2 x gate in, 1 x gate out
#define NOF_BUTTONS                  2 + 2 + 1
//#define USE_ENCODERS
#define NOF_ENCODERS                 1
//#define ENCODER_TIM1 htim4

#define USE_USBD_AUDIO
#define USE_CODEC
#define USE_AK4556
#define USE_USBD_FS
#define USBD_HANDLE hUsbDeviceFS
#define AUDIO_BITS_PER_SAMPLE       16
#define AUDIO_BYTES_PER_SAMPLE      (AUDIO_BITS_PER_SAMPLE/8)
#define AUDIO_CHANNELS              2
#define USB_AUDIO_CHANNELS          2

/*
#define USE_UART_MIDI
#define UART_MIDI_HANDLE huart1
#define UART_MIDI_RX_BUFFER_SIZE 256
*/

/* We shouldn't need to touch QSPI - it's initialized by bootloader */
/*
#define QSPI_HANDLE hqspi
//#define QSPI_DEVICE_IS25LP080D
#define QSPI_DEVICE_IS25LP064A
*/

//#define USE_IWDG
//#define IWDG_PERIPH IWDG1
/* #define INIT_FMC */
