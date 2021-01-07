#include "main.h"
#include "stm32l4xx_hal.h"

//#define OWL_MAGUS
#define HARDWARE_ID                  NEOPHYTE_HARDWARE
#define HARDWARE_VERSION             "Neophyte"

//#define USE_SCREEN
//#define SSD1309
/* #define OLED_DMA */
//#define OLED_SOFT_CS
//#define OLED_SPI hspi6
/* #define OLED_IT */
/* #define OLED_BITBANG */

//#define MAIN_LOOP_SLEEP_MS          20

#define MAX11300_SPI hspi5
#define ENCODERS_SPI hspi5
#define NO_EXTERNAL_RAM
#define USE_FIRMWARE_HEADER

#define USE_USBD_FS

#define NO_FFT_TABLES

#undef USE_ADC
#define NOF_ADC_VALUES               0
#define NOF_PARAMETERS               20
#define NOF_BUTTONS                  0
#undef USE_DAC

#define AUDIO_INPUT_OFFSET           ((uint32_t)(0.00007896*65535))
#define AUDIO_INPUT_SCALAR           ((uint32_t)(-6.43010423*65535))
#define AUDIO_OUTPUT_OFFSET          ((uint32_t)(0.00039729*65535))
#define AUDIO_OUTPUT_SCALAR          ((uint32_t)(5.03400000*65535))
