#include "main.h"
#include "stm32f4xx_hal.h"

#define OWL_MAGUS
#define HARDWARE_ID                  MAGUS_HARDWARE
#define HARDWARE_VERSION             "Magus"

#ifdef USE_DIGITALBUS
#define DIGITAL_BUS_ENABLED          1
#define DIGITAL_BUS_FORWARD_MIDI     0
#define BUS_HUART huart2
#endif

#define USE_SCREEN
#define SSD1309
/* #define OLED_DMA */
#define OLED_SOFT_CS
#define OLED_SPI hspi6
/* #define OLED_IT */
/* #define OLED_BITBANG */
#define USE_CODEC
#define USE_CS4271
#define CODEC_SPI hspi4
#define USE_USBD_FS
#define USE_USB_HOST
#define USBH_HANDLE hUsbHostHS
#define USB_HOST_RX_BUFF_SIZE        256  /* Max Received data 64 bytes */

#define TLC5946_SPI hspi6
#define MAX11300_SPI hspi5
#define ENCODERS_SPI hspi5

#undef USE_ADC
#define NOF_ADC_VALUES               0
#define NOF_PARAMETERS               20
#define NOF_BUTTONS                  0
#undef USE_DAC

