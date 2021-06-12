#include "main.h"
#include "stm32h7xx_hal.h"

#define OWL_PREENFM3
#define HARDWARE_ID                  OTHER_HARDWARE
#define HARDWARE_VERSION             "PreenFM3"

/* #define NO_EXTERNAL_RAM */
/* #define NO_CCM_RAM */
#define DMA_RAM                      __attribute__ ((section (".dmadata")))
#define GRAPHICS_RAM                 __attribute__ ((section (".graphics")))

//#define USE_PLUS_RAM
#define USE_ICACHE
#define USE_DCACHE

#define MAX_SYSEX_PROGRAM_SIZE      (512*1024)

#define hdac hdac1

#define USE_SCREEN
#define ILI9341

#define OLED_DMA
#define OLED_SOFT_CS
#define OLED_SPI hspi1
#define SCREEN_BUFFER_OFFSET (240 * 40 * 2)
#define SCREEN_HEIGHT_OFFSET 100 // 40 px top + 60 px bottom
#define USE_CODEC
#define MULTI_CODEC
#define USE_CS4344
#define HSAI_TX1 hsai_BlockA1
#define HSAI_TX2 hsai_BlockB1
#define HSAI_TX3 hsai_BlockA2
#define HSAI_TX_NUMBER              3

/* USB audio settings */
#define AUDIO_BITS_PER_SAMPLE       16
#define AUDIO_BYTES_PER_SAMPLE      (AUDIO_BITS_PER_SAMPLE/8)
#define AUDIO_CHANNELS              6
#define USBD_AUDIO_RX_CHANNELS      0
#define USBD_AUDIO_TX_CHANNELS      AUDIO_CHANNELS
#define AUDIO_INT32_TO_SAMPLE(x)    ((x)>>8)
#define AUDIO_SAMPLE_TO_INT32(x)    ((int32_t)(x)<<8)

#define MAIN_LOOP_SLEEP_MS          1
#define ARM_CYCLES_PER_SAMPLE       (480000000/AUDIO_SAMPLINGRATE) /* 480MHz / 48kHz */

#define USE_USBD_AUDIO
#define USE_USBD_AUDIO_TX  // microphone
#define USE_USBD_AUDIO_RX // speaker
#define USE_USBD_FS
#define USBD_HANDLE hUsbDeviceFS

#define NO_EXTERNAL_RAM

// Serial MIDI
#define USE_UART_MIDI_RX
#define USE_UART_MIDI_TX
#define UART_MIDI_HANDLE huart1
#define UART_MIDI_RX_BUFFER_SIZE 256
// Digital bus
/* #define USE_DIGITALBUS */
/* #define DIGITAL_BUS_ENABLED 1 */
/* #define DIGITAL_BUS_FORWARD_MIDI 0 */
/* #define BUS_HUART huart2 */

#define NOF_PARAMETERS               40
#define NOF_BUTTONS                  (4+6)
#define NOF_ENCODERS                 6

#define PFM_SET_PIN(x, y)   x->BSRR = y
#define PFM_CLEAR_PIN(x, y)  x->BSRR = (uint32_t)y << 16
#define PREENFM_FREQUENCY 47916.0f
