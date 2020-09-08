#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "hardware.h"

#define FIRMWARE_VERSION "v21.0"

#ifndef AUDIO_OUTPUT_GAIN
#define AUDIO_OUTPUT_GAIN            112
#endif
#ifndef AUDIO_INPUT_GAIN
#define AUDIO_INPUT_GAIN             92
#endif

#define MIDI_OMNI_CHANNEL            (-1)
#ifndef MIDI_INPUT_CHANNEL
#define MIDI_INPUT_CHANNEL           MIDI_OMNI_CHANNEL
#endif
#ifndef MIDI_OUTPUT_CHANNEL
#define MIDI_OUTPUT_CHANNEL          0
#endif
#ifndef DIGITAL_BUS_ENABLED
#define DIGITAL_BUS_ENABLED          0
#define DIGITAL_BUS_FORWARD_MIDI     0
#endif
#define USE_USBD_MIDI
#define USE_MIDI_TX_BUFFER
#define USE_MIDI_CALLBACK
#define MIDI_OUTPUT_BUFFER_SIZE      128
#define MIDI_INPUT_BUFFER_SIZE       64
#define MIDI_SYSEX_BUFFER_SIZE       256

#ifndef USBD_MAX_POWER
#define USBD_MAX_POWER               200 // 200mA
#endif

#define OWLBOOT_MAGIC_NUMBER        (0xDADAB007)
#define OWLBOOT_MAGIC_ADDRESS       ((uint32_t*)0x2000FFF0)
#define OWLBOOT_COMMAND_NUMBER      (0xD0D0B008)
#define OWLBOOT_COMMAND_ADDRESS     ((uint32_t*)0x3800FFE0)

#ifndef STORAGE_MAX_BLOCKS
#define STORAGE_MAX_BLOCKS           64
#endif

#define DEBUG_DWT
/* #define DEBUG_STACK */
/* #define DEBUG_STORAGE */

#ifdef SSD1331
#define OLED_WIDTH		     96
#define OLED_HEIGHT		     64
#define OLED_BUFFER_SIZE             (OLED_WIDTH*OLED_HEIGHT/8)
#elif defined SEPS114A
#define OLED_WIDTH		     96
#define OLED_HEIGHT		     96
#define OLED_BUFFER_SIZE             (OLED_WIDTH*OLED_HEIGHT*sizeof(uint16_t))
#elif defined SSD1309
#define OLED_WIDTH		     128
#define OLED_HEIGHT	             64
#define OLED_BUFFER_SIZE             (OLED_WIDTH*OLED_HEIGHT/8)
#endif

#ifndef MAX_SYSEX_FIRMWARE_SIZE
#define MAX_SYSEX_FIRMWARE_SIZE      ((16+16+64+128+128)*1024) // FLASH sectors 2-6
#endif
#ifndef MAX_SYSEX_PROGRAM_SIZE
#define MAX_SYSEX_PROGRAM_SIZE       (128*1024) // 128k, one flash sector
#endif
#define MAX_FACTORY_PATCHES          36
#define MAX_USER_PATCHES             4

#define MAX_NUMBER_OF_PATCHES        40
#define MAX_NUMBER_OF_RESOURCES      12

#define CODEC_BLOCKSIZE              64
#define CODEC_BUFFER_SIZE            (2*AUDIO_CHANNELS*CODEC_BLOCKSIZE)

/* +0db in and out */
#define AUDIO_INPUT_OFFSET           0xffffefaa /* -0.06382 * 65535 */
#define AUDIO_INPUT_SCALAR           0xfffbb5c7 /* -4.290 * 65535 */
#define AUDIO_OUTPUT_OFFSET          0x00001eec /* 0.1208 * 65535 */
#define AUDIO_OUTPUT_SCALAR          0xfffb5bab /* -4.642 * 65535 */
#define DEFAULT_PROGRAM              1
#define BUTTON_PROGRAM_CHANGE
#define AUDIO_BITDEPTH               24    /* bits per sample */
#define AUDIO_DATAFORMAT             24
#define AUDIO_CODEC_MASTER           true
#ifndef AUDIO_CHANNELS
#define AUDIO_CHANNELS               2
#endif
#ifndef AUDIO_SAMPLINGRATE
#define AUDIO_SAMPLINGRATE           48000
#endif
#define AUDIO_BLOCK_SIZE             CODEC_BLOCKSIZE   /* size in samples of a single channel audio block */
#define AUDIO_MAX_BLOCK_SIZE         (CODEC_BUFFER_SIZE/4)

#ifndef DAC_PERIPH
#define DAC_PERIPH hdac
#endif

#define PROGRAM_TASK_STACK_SIZE      (4*1024/sizeof(portSTACK_TYPE))
#define MANAGER_TASK_STACK_SIZE      (1024/sizeof(portSTACK_TYPE))
#define FLASH_TASK_STACK_SIZE        (512/sizeof(portSTACK_TYPE))
#define UTILITY_TASK_STACK_SIZE      (512/sizeof(portSTACK_TYPE))
#ifdef DAISY
#define ARM_CYCLES_PER_SAMPLE        (400000000/AUDIO_SAMPLINGRATE) /* 400MHz / 48kHz */
#else
#define ARM_CYCLES_PER_SAMPLE        (168000000/AUDIO_SAMPLINGRATE) /* 168MHz / 48kHz */
#endif

#define CCM                          __attribute__ ((section (".ccmdata")))

/* #define USE_IWDG                     // compile with support for IWDG watchdog */
#if defined(USE_IWDG) && !defined(IWDG_PERIPH)
#define IWDG_PERIPH IWDG
#endif

#endif /* __DEVICE_H__ */
