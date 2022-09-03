#include "pcm3060.h"
#include "gpio.h"
#include "device.h"
#include "main.h"
#include "errorhandlers.h"

#define CODEC_TIMEOUT       250

#ifndef CODEC_ADDRESS
  // bit 1 can be set via hardware and should be configurable.
  #define CODEC_ADDRESS       0x8c
#endif

#define REG_SYS_CTRL        0x40
#define REG_DAC_ATTEN_LEFT  0x41
#define REG_DAC_ATTEN_RIGHT 0x42
#define REG_DAC_CTRL1       0x43
#define REG_DAC_CTRL2       0x44
#define REG_DIG_CTRL        0x45
#define REG_ADC_ATTEN_LEFT  0x46
#define REG_ADC_ATTEN_RIGHT 0x47
#define REG_ADC_CTRL1       0x48
#define REG_ADC_CTRL2       0x49

// SysCtrl masks
// Reset masks
#define MRST_BIT_MASK    0x80
#define SRST_BIT_MASK    0x40
#define ADC_PSV_BIT_MASK 0x20
#define DAC_PSV_BIT_MASK 0x10
#define FMT_BIT_MASK     0x03

void codec_write(uint8_t reg, uint8_t data){
  extern I2C_HandleTypeDef CODEC_I2C;
  while(HAL_I2C_GetState(&CODEC_I2C) != HAL_I2C_STATE_READY) {};  
  if (HAL_I2C_Mem_Write(&CODEC_I2C, CODEC_ADDRESS, reg, 1, &data, 1, CODEC_TIMEOUT) != HAL_OK)
    error(CONFIG_ERROR, "Codec error");
}


uint8_t codec_read(uint8_t reg){
  extern I2C_HandleTypeDef CODEC_I2C;
  uint8_t data;
  while(HAL_I2C_GetState(&CODEC_I2C) != HAL_I2C_STATE_READY) {};  
  if (HAL_I2C_Mem_Read(&CODEC_I2C, CODEC_ADDRESS, reg, 1, &data, 1, CODEC_TIMEOUT) != HAL_OK)
    error(CONFIG_ERROR, "Codec error");  
  return data;
}

void codec_reset(){
  // MRST
  uint8_t sysreg = codec_read(REG_SYS_CTRL);
  sysreg &= ~(MRST_BIT_MASK);
  codec_write(REG_SYS_CTRL, sysreg);
  HAL_Delay(4);

  /* codec_write(64, PCM3168A_MRST|PCM3168A_SRST); */
/* This bit controls system reset, the relation between system clock and sampling clock re-synchronization, and ADC */
/* operation and DAC operation restart. The mode control register is not reset and the PCM3168A device does not go into a */
/* power-down state. The fade-in sequence is supported in the resume process, but pop-noise may be generated. Returning */
/* the SRST bit to 1 is unneccesary; it is automatically set to 1 after triggering a system reset. */
}

// todo: remove
//uint32_t pcm3168a_dz = 0;
//uint32_t pcm3168a_adc_ovf = 0;
//uint32_t pcm3168a_check = 0;

void codec_init(){
  // MRST  
  uint8_t sysreg = codec_read(REG_SYS_CTRL);
  sysreg &= ~(MRST_BIT_MASK);
  codec_write(REG_SYS_CTRL, sysreg);
  HAL_Delay(4);

  // SRST
  sysreg = codec_read(REG_SYS_CTRL);
  sysreg &= ~(SRST_BIT_MASK);
  codec_write(REG_SYS_CTRL, sysreg);
  HAL_Delay(4);

  // ADC/DAC Format set to 24-bit LJ
  uint8_t dac_ctrl, adc_ctrl;
  dac_ctrl = codec_read(REG_DAC_CTRL1);
  adc_ctrl = codec_read(REG_ADC_CTRL1);
  dac_ctrl |= (FMT_BIT_MASK & 1);
  adc_ctrl |= (FMT_BIT_MASK & 1);
  codec_write(REG_DAC_CTRL1, dac_ctrl);
  codec_write(REG_ADC_CTRL1, adc_ctrl);

  // Disable Powersave for ADC/DAC
  sysreg = codec_read(REG_SYS_CTRL);
  sysreg &= ~(ADC_PSV_BIT_MASK);
  sysreg &= ~(DAC_PSV_BIT_MASK);
  codec_write(REG_SYS_CTRL, sysreg);

}

void codec_bypass(int bypass){
  // todo
}

void codec_mute(bool mute){  
  /* Register 68: DAC Soft Mute Control */
  //codec_write(68, mute ? 0xff : 0x00);
}

/**
 * Adjustable from 127 = +20dB to -113 = -100dB in 0.5dB steps. Less than -113 is mute.
 */
void codec_set_gain_in(int8_t level){
  /* Register 88: ADC Digital attenuation level setting */
  //uint8_t value = level+128;
  //codec_write(88, value); // 
}

/**
 * Adjustable from 127 = 0dB to -73 = -100dB in 0.5dB steps. Less than -73 is mute.
 */
void codec_set_gain_out(int8_t level){
  /* Register 71: DAC Digital attenuation level setting */
  //uint8_t value = level+128;
  //codec_write(71, value); // 
}

