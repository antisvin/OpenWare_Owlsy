#include "device.h"
#include "wm8731.h"
#include "errorhandlers.h"

#include "gpio.h"

bool is_seed_11 = false;

bool get_seed_11() {
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  is_seed_11 = !getPin(GPIOD, GPIO_PIN_3);
  return is_seed_11;
}


void codec_write(uint8_t reg, uint16_t value){
/* void codec_write(uint8_t reg, uint8_t data){ */

  /* uint8_t Byte1 = ((RegisterAddr<<1)&0xFE) | ((RegisterValue>>8)&0x01); */
  /* uint8_t Byte2 = RegisterValue&0xFF; */

  /* Assemble 2-byte data in WM8731 format */
  uint8_t data[2] = {
    ((reg<<1)&0xFE) | ((value>>8)&0x01), value&0xFF   
  };

  extern I2C_HandleTypeDef WM8731_I2C_HANDLE;
  HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&WM8731_I2C_HANDLE, CODEC_ADDRESS, data, 2, 10000);
  if(ret != HAL_OK)
    error(CONFIG_ERROR, "I2C transmit failed");
  /* HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) */
}

void codec_reset(){
  codec_write(RESET_CONTROL_REGISTER, 0); // reset
}

void codec_init(){
  extern bool is_seed_11;

  if (is_seed_11) {
    // WM8371
    /* Load default values */
    for(int i=0;i<WM8731_NUM_REGS-1;i++)
      codec_write(i, wm8731_init_data[i]);

    // set WM8731_MS master mode
    codec_write(DIGITAL_AUDIO_INTERFACE_FORMAT_REGISTER, WM8731_FORMAT_MSB_LJ|WM8731_IWL_24BIT);

    // clear OSCPD and OUTPD and CLKOUTPD
    codec_write(POWER_DOWN_CONTROL_REGISTER, WM8731_MICPD);

    // set active control
    codec_write(ACTIVE_CONTROL_REGISTER, WM8731_ACTIVE);
  }
  else {
    // AK4556
    clearPin(CODEC_RESET1_GPIO_Port, CODEC_RESET1_Pin);
    HAL_Delay(1);
    // Datasheet specifies minimum 150ns delay. We don't really need it as codec would be
    // running longer than that by now, but better safe than sorry.
    setPin(CODEC_RESET1_GPIO_Port, CODEC_RESET1_Pin);    
  }

#if defined(DAISY) && defined(DUAL_CODEC)
  // very hacky - init the second AK4556 here for Daisy Patch
    clearPin(CODEC_RESET2_GPIO_Port, CODEC_RESET2_Pin);
    HAL_Delay(1);
    // Datasheet specifies minimum 150ns delay. We don't really need it as codec would be
    // running longer than that by now, but better safe than sorry.
    setPin(CODEC_RESET2_GPIO_Port, CODEC_RESET2_Pin);
#endif
}

void codec_bypass(int bypass){
  extern bool is_seed_11;

  if (is_seed_11) {  
    uint16_t value = WM8731_MUTEMIC;
    if(bypass)
      value |= WM8731_BYPASS;
    else
      value |= WM8731_DACSEL;
    codec_write(ANALOGUE_AUDIO_PATH_CONTROL_REGISTER, value);
  }
}

/* Set input gain between 0 (mute) and 127 (max) */
void codec_set_gain_in(int8_t level){
  extern bool is_seed_11;

  if (is_seed_11) {
    level = level >> 2;
    uint16_t gain = (wm8731_init_data[LEFT_LINE_IN_REGISTER] & 0xe0) | (level & 0x1f);
    codec_write(LEFT_LINE_IN_REGISTER, gain);
    gain = (wm8731_init_data[RIGHT_LINE_IN_REGISTER] & 0xe0) | (level & 0x1f);
    codec_write(RIGHT_LINE_IN_REGISTER, gain);
  }
}

void codec_set_gain_out(int8_t level){
  extern bool is_seed_11;

  if (is_seed_11) {
    uint16_t gain = (wm8731_init_data[LEFT_HEADPHONE_OUT_REGISTER] & 0x80) | (level & 0x7f);
    codec_write(LEFT_HEADPHONE_OUT_REGISTER, gain);
    gain = (wm8731_init_data[RIGHT_HEADPHONE_OUT_REGISTER] & 0x80) | (level & 0x7f);
    codec_write(RIGHT_HEADPHONE_OUT_REGISTER, gain);
  }
}
