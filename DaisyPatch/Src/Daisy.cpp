#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#include "Daisy.h"
#include "device.h"
#include "Owl.h"
#include "Graphics.h"
//#include "errorhandlers.h"
#include "message.h"
#include "gpio.h"
#include "qspicontrol.h"
#include "ProgramManager.h"
#include "PatchRegistry.h"
#include "OpenWareMidiControl.h"
#include "SoftwareEncoder.hpp"


static SoftwareEncoder encoder(
  ENC_A_GPIO_Port, ENC_A_Pin,
  ENC_B_GPIO_Port, ENC_B_Pin, 
  ENC_CLICK_GPIO_Port, ENC_CLICK_Pin);


extern "C" void updateEncoderCounter(){
  encoder.updateCounter();
}

void setGateValue(uint8_t ch, int16_t value){
  switch(ch){
  case PUSHBUTTON:
  case BUTTON_A:
    HAL_GPIO_WritePin(GATE_OUT_GPIO_Port, GATE_OUT_Pin, value ? GPIO_PIN_SET :  GPIO_PIN_RESET);
    break;
  }
}


extern uint16_t adc_values[NOF_ADC_VALUES];

/*
void updateParameters(){
#ifdef USE_CACHE
  SCB_InvalidateDCache_by_Addr((uint32_t*)((uint32_t)(adc_values) & ~(uint32_t)0x1F), sizeof(adc_values));
#endif
  for(int i = 0; i < NOF_ADC_VALUES; ++i)
    graphics.params.updateValue(i, 4095 - adc_values[i]);
}
*/

void setup(){
  HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET); // OLED off

  if (qspi_init(QSPI_MODE_MEMORY_MAPPED) != MEMORY_OK){
    // We can end here only if QSPI settings are misconfigured
    error(RUNTIME_ERROR, "Flash init error");
  }

  extern SPI_HandleTypeDef OLED_SPI;
  graphics.begin(&OLED_SPI);

  owl_setup();
}

extern uint16_t adc_values[NOF_ADC_VALUES];
static int16_t enc_data[2];

void loop(void){
#ifdef USE_CACHE
  SCB_InvalidateDCache_by_Addr((uint32_t*)((uint32_t)(adc_values) & ~(uint32_t)0x1F), sizeof(adc_values));
  #ifdef USE_SCREEN
  SCB_CleanInvalidateDCache_by_Addr((uint32_t*)((uint32_t)(&graphics.params) & ~(uint32_t)0x1F), sizeof(graphics.params));
  #endif
#endif

  encoder.debounce();
  enc_data[0] = int16_t(encoder.isFallingEdge() | encoder.isLongPress() << 1);
  enc_data[1] = int16_t(encoder.getValue());
  graphics.params.updateEncoders(enc_data, 1);


  for(int i = 0; i < NOF_ADC_VALUES; ++i)
    graphics.params.updateValue(i, 4095 - (uint16_t(getAnalogValue(i))));

  for(int i = NOF_ADC_VALUES; i < NOF_PARAMETERS; ++i) {
    graphics.params.updateValue(i, 0);
  }

  
  owl_loop();
}
