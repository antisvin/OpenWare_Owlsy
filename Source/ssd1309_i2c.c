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

#define OLED_DAT 1
#define OLED_CMD 0

// _____ Prototypes
// ____________________________________________________________________
static void OLED_writeCMD(const uint8_t *data, uint16_t length);

// _____ Variables
// _____________________________________________________________________
static const uint8_t OLED_initSequence[] = {
#if 0
	0xfd, 0x12, 	// Command unlock
	0xae, 		// Display off

    0x20, 0b01,      // Set Memory Addressing Mode
    // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
    // 10=Page Addressing Mode (RESET); 11=Invalid
    0xB0,            // Set Page Start Address for Page Addressing Mode, 0-7
    0xC8,            // Set COM Output Scan Direction
    0x00,            // --set low column address
    0x10,            // --set high column address
    0x40,            // --set start line address
    0x81, 0x3F,      // Set contrast control register
    0xA0,            // Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
    0xA6,            // Set display mode. A6=Normal; A7=Inverse
    0xA8, 0x1f, // Set multiplex ratio(1 to 64)
    0xA4,            // Output RAM to Display
					 // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
    0xD3, 0x00,      // Set display offset. 00 = no offset
    0xD5,            // --set display clock divide ratio/oscillator frequency
    0x80,            // --set divide ratio
    0xD9, 0x22,      // Set pre-charge period
		     // Set com pins hardware configuration
    //0xDA, 0x12,      
    0xDA, 0x02,
    0xDB,            // --set vcomh
    0x20,            // 0x20,0.77xVcc
    0x8D, 0x14,      // Set DC-DC enable
#else
    // 0xfd, 0x12, // Command unlock
    0xae,       // Display off
    0xd5, 0xa0, // Clock divide ratio / Oscillator Frequency
                //    0xd5, 0xa0, // Clock divide ratio / Oscillator Frequency
    0xa8, 0x1f, // Multiplex ratio 32
    0xd3, 0x00, // Display offset
    0x40,       // Start line
    0x8d, 0x14, // Charge pump

#ifdef OLED_UPSIDE_DOWN
    0xc1, // Scan direction: c0: scan dir normal, c8: reverse
    0xa0, // Segment re-map: a0: col0 -> SEG0, a1: col127 -> SEG0
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
    0x21, 0x20, 0x5f, // Set column address
    0x22, 0x00, 0x03, // Set page address
    0xaf,             // Display on
#endif
};

/* static unsigned char* OLED_Buffer; */
static I2C_HandleTypeDef *OLED_I2CInst;

// _____ Functions
// _____________________________________________________________________
static void OLED_writeCMD(const uint8_t *data, uint16_t length) {
  // Send Data
  while (OLED_I2CInst->State != HAL_I2C_STATE_READY) {
  }; // wait
  uint8_t buf[2] = {0X0};
  for (int i = 0; i < length; i++) {
    buf[1] = data[i];
    HAL_I2C_Master_Transmit(OLED_I2CInst, OLED_I2C_ADDRESS << 1, buf, 2, 1000);
  }
  delay(1);
}

#ifdef OLED_DMA
#if 0
// TODO: I2C support
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->ErrorCode == HAL_SPI_ERROR_OVR) {
    __HAL_SPI_CLEAR_OVRFLAG(hspi);
    error(RUNTIME_ERROR, "SPI OVR");
  } else {
    assert_param(0);
  }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  /* if(__HAL_DMA_GET_FLAG(OLED_SPIInst,
   * __HAL_DMA_GET_TC_FLAG_INDEX(OLED_SPIInst))){ */
  /*   pCS_Set();	// CS high */
  /* } */
  pCS_Set(); // CS high
}
#endif
#endif

void oled_write(const uint8_t *data, uint16_t length) {
#ifdef OLED_DMA
#ifdef USE_CACHE
  SCB_CleanInvalidateDCache_by_Addr((uint32_t *)data, length);
#endif
  while (OLED_I2CInst->State != HAL_I2C_STATE_READY)
    ; // wait

  HAL_I2C_Mem_Write_DMA(OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x40, 1, data,
                        length);
#else
/*
  uint8_t buf[] = {
    0x21, 0x20, 0x5f, // Set column address
    0x22, 0x00, 0x03, // Set page address
    0x40
  };
  OLED_writeCMD(buf, sizeof(buf));

*/
   while (OLED_I2CInst->State != HAL_I2C_STATE_READY) {}; // wait
   HAL_I2C_Mem_Write(
       OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x40, 1, data, length, 1000);
   //HAL_I2C_Master_Transmit(OLED_I2CInst, OLED_I2C_ADDRESS << 1, data, length, 1000);
#if 0
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t buf[] = {0xB0 + i, 0x00, 0x12};
    OLED_writeCMD(buf, 3);
    //(hi2c,
    // ssd1306_WriteCommand(hi2c, 0x00);
    // ssd1306_WriteCommand(hi2c, 0x10);

    HAL_I2C_Mem_Write(OLED_I2CInst, OLED_I2C_ADDRESS << 1, 0x40, 1,
                      data + (OLED_WIDTH * i), OLED_WIDTH, 1000);
  }
#endif
  // uint8_t buf = {0x40};
  // HAL_I2C_Master_Transmit(OLED_I2CInst, OLED_I2C_ADDRESS << 1, buf, 1, 1000);

  // HAL_I2C_Master_Transmit(OLED_I2CInst, OLED_I2C_ADDRESS << 1, (uint8_t
  // *)data,
//                          length, 1000);
// uint8_t buf[2] = {0X40};
// HAL_I2C_Master_Transmit(OLED_I2CInst, OLED_I2C_ADDRESS << 1, buf, 2, 1000);
// while (OLED_I2CInst->State != HAL_I2C_STATE_READY) {}; // wait
// HAL_I2C_Master_Transmit(OLED_I2CInst, OLED_I2C_ADDRESS << 1, data, length,
// 1000); for (int i = 0; i < length; i++) {
// uint8_t buf[2] = {0X40, data[i]};
// buf[1] = data[i];
// HAL_I2C_Master_Transmit(OLED_I2CInst, OLED_I2C_ADDRESS << 1, buf, 2, 1000);
//}
#endif
}

// Configuration
void oled_init(I2C_HandleTypeDef *i2c) {
  OLED_I2CInst = i2c;
  // Initialisation
  delay(5);
  while (OLED_I2CInst->State != HAL_I2C_STATE_READY) {}; // wait
  OLED_writeCMD(OLED_initSequence, sizeof OLED_initSequence);
  delay(10);
}
