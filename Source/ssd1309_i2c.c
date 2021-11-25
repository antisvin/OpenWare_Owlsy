// _____ Includes
// ______________________________________________________________________
#include "device.h"
#include "errorhandlers.h"
#include "oled.h"
#include <string.h>

/* static void NopDelay(uint32_t nops){ */
/*   while (nops--) */
/*     __asm("NOP"); */
/* } */
/* #define delay(x) NopDelay(x*1000) */
#define delay(x) HAL_Delay(x)
/* #define delay(x) osDelay(x) */


// _____ Prototypes
// ____________________________________________________________________
static void OLED_writeCMD(const uint8_t *data, uint16_t length);

// _____ Variables
// _____________________________________________________________________
static const uint8_t OLED_initSequence[] = {
    // 0xfd, 0x12, // Command unlock
    0xae,       // Display off
    0xd5, 0xa0, // Clock divide ratio / Oscillator Frequency
                //    0xd5, 0xa0, // Clock divide ratio / Oscillator Frequency
    0xa8, 0x1f, // Multiplex ratio 32
    0xd3, 0x00, // Display offset
    0x40,       // Start line
    0x8d, 0x14, // Charge pump

#ifdef OLED_UPSIDE_DOWN
    0xc1,       // Scan direction: c0: scan dir normal, c8: reverse
    0xa0,       // Segment re-map: a0: col0 -> SEG0, a1: col127 -> SEG0
#else
    0xc8, 0xa1,
#endif
    // 0xda, 0x00, // COM pins
    0xda, 0x12, // COM pins
    0xd9, 0x25, // Pre-charge period
                //    0xd9, 0x22, // Pre-charge period
    0xdb, 0x34, // VCOMH deselect level
    0xa6,       // Normal / inverse display
    0xa4,       // Entire display on/off */
    0x81, 0x8f, // Contrast control: 0 to 0xff. Current increases with contrast.
    //    0x81, 0xcf, // Contrast control: 0 to 0xff. Current increases with
    //    contrast.

    0x20, 0x01,       // Vertical addressing mode
    0x21, 0x20, 0x5f, // Set column address to 64
    0x22, 0x00, 0x03, // Set page address to 32
    0xaf,             // Display on
};

/* static unsigned char* OLED_Buffer; */
static I2C_HandleTypeDef *OLED_I2CInst;

// _____ Functions
// _____________________________________________________________________
static void OLED_writeCMD(const uint8_t *data, uint16_t length) {
  // Send Data
  #ifdef OLED_DMA
  HAL_I2C_Mem_Write_DMA(OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x00, 1, data, length);
  #else
  HAL_I2C_Mem_Write(OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x00, 1, data, length, 1000);
  #endif
}

#ifdef OLED_DMA
//void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {};
#endif

void oled_write(const uint8_t *data, uint16_t length) {
#ifdef OLED_DMA
#ifdef USE_CACHE
  SCB_CleanInvalidateDCache_by_Addr((uint32_t *)data, length);
#endif
//  HAL_I2C_Mem_Write(OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x40, 1, data, length, 1000);
  HAL_I2C_Mem_Write_DMA(OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x40, 1, data, length);
#else
  HAL_I2C_Mem_Write(OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x40, 1, data, length, 1000);  
#endif
}

// Configuration
void oled_init(I2C_HandleTypeDef *i2c) {
  OLED_I2CInst = i2c;
  // Initialisation
  delay(5);
  while (OLED_I2CInst->State != HAL_I2C_STATE_READY) {
  }; // wait
  OLED_writeCMD(OLED_initSequence, sizeof OLED_initSequence);
  delay(10);
}
