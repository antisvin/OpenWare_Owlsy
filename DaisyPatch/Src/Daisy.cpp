#include <string.h>
#include <stdlib.h>
#include "device.h"
#include "Owl.h"
#include "Graphics.h"
//#include "errorhandlers.h"
#include "message.h"
#include "ProgramManager.h"
#include "PatchRegistry.h"
#include "OpenWareMidiControl.h"
#include "SoftwareEncoder.hpp"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef abs
#define abs(x) ((x)>0?(x):-(x))
#endif

static int16_t counter;

static SoftwareEncoder<> encoder(
  ENC_A_GPIO_Port, ENC_A_Pin,
  ENC_B_GPIO_Port, ENC_B_Pin, 
  ENC_CLICK_GPIO_Port, ENC_CLICK_Pin, (uint16_t*)&counter);

void setGateValue(uint8_t ch, int16_t value){
/*
  switch(ch){
  case PUSHBUTTON:
  case BUTTON_A:
    HAL_GPIO_WritePin(TR_OUT1_GPIO_Port, TR_OUT1_Pin, value ? GPIO_PIN_RESET :  GPIO_PIN_SET);
    break;
  case BUTTON_B:
    HAL_GPIO_WritePin(TR_OUT2_GPIO_Port, TR_OUT2_Pin, value ? GPIO_PIN_RESET :  GPIO_PIN_SET);
    break;
  }
*/
}

bool isModeButtonPressed(){
//  return HAL_GPIO_ReadPin(SW5_GPIO_Port, SW5_Pin) == GPIO_PIN_RESET;
   return false;
}

void setup(){
/*
  HAL_GPIO_WritePin(TR_OUT1_GPIO_Port, TR_OUT1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TR_OUT2_GPIO_Port, TR_OUT2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDPWM1_GPIO_Port, LEDPWM1_Pin, GPIO_PIN_SET);
*/
  HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET); // OLED off
  extern SPI_HandleTypeDef OLED_SPI;
  graphics.begin(&OLED_SPI);

  owl_setup();
}

void loop(void){
  encoder.update();
  setButtonValue(BUTTON_A, encoder.isPressed());
  setButtonValue(PUSHBUTTON, encoder.isPressed());
  setButtonValue(BUTTON_B, encoder.isLongCLick());
  graphics.params.updateEncoders(&counter, 1);

  for(int i = 0; i < NOF_ADC_VALUES; ++i)
    graphics.params.updateValue(i, 4095 - (uint16_t(getAnalogValue(i)) >> 4));
  
  owl_loop();
}
