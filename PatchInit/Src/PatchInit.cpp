#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "device.h"
#include "Owl.h"
//#include "errorhandlers.h"
#include "message.h"
#include "gpio.h"
#include "qspicontrol.h"
#include "ProgramManager.h"
#include "PatchRegistry.h"
#include "OpenWareMidiControl.h"
#include "SoftwareEncoder.hpp"
#include "VersionToken.h"

extern bool updateUi;
extern uint16_t button_values;

void setGateValue(uint8_t bid, int16_t value){
  if (bid == BUTTON_C || bid == PUSHBUTTON) {
    HAL_GPIO_WritePin(GATE_OUT1_GPIO_Port, GATE_OUT1_Pin, value ? GPIO_PIN_SET :  GPIO_PIN_RESET);
  }
  else if (bid == BUTTON_D) {
    HAL_GPIO_WritePin(GATE_OUT2_GPIO_Port, GATE_OUT2_Pin, value ? GPIO_PIN_SET :  GPIO_PIN_RESET);
  }
}

void setAnalogValue(uint8_t ch, int16_t value){
  extern DAC_HandleTypeDef DAC_PERIPH;
  switch(ch){
  case PARAMETER_F:
    HAL_DAC_SetValue(&DAC_PERIPH, DAC_CHANNEL_1, DAC_ALIGN_12B_R, __USAT(value, 12));
    break;
  case PARAMETER_G:
    HAL_DAC_SetValue(&DAC_PERIPH, DAC_CHANNEL_2, DAC_ALIGN_12B_R, __USAT(value, 12));
    break;
  }
}

void updateParameters(int16_t* parameter_values, size_t parameter_len, uint16_t* adc_values, size_t adc_len){
#ifdef USE_CACHE
  //SCB_InvalidateDCache_by_Addr((uint32_t*)adc_values, sizeof(adc_values));
#endif
  // Note that updateValue will apply smoothing, so we don't have to do it here
  /*
  graphics.params.updateValue(0, 4095 - adc_values[0]);
  graphics.params.updateValue(1, 4095 - adc_values[1]);
  graphics.params.updateValue(2, 4095 - adc_values[2]);
  graphics.params.updateValue(3, 4095 - adc_values[3]);
  */
}

void onChangePin(uint16_t pin){
  switch(pin){
  case GATE_IN1_Pin: {
    bool state = !(GATE_IN1_GPIO_Port->IDR & GATE_IN1_Pin);
    setButtonValue(BUTTON_A, state);
    setButtonValue(PUSHBUTTON, state);
    break;
  }
  case GATE_IN2_Pin:
    setButtonValue(BUTTON_B, !(GATE_IN2_GPIO_Port->IDR & GATE_IN2_Pin));
    break;
  default:
    break;
  }
}

void setup(){
  qspi_init(QSPI_MODE_MEMORY_MAPPED);
  
  /* This doesn't work with bootloader, need to find how to deinit it earlier*/
  #if 0
  if (qspi_init(QSPI_MODE_MEMORY_MAPPED) != MEMORY_OK){
    // We can end here only if QSPI settings are misconfigured
    error(RUNTIME_ERROR, "Flash init error");
  }
  #endif

  owl.setup();
  setButtonValue(PUSHBUTTON, 0);
}

static int16_t enc_data[2];

void loop(){
/*
  updateEncoderCounter();
  if (updateUi){
#if defined USE_CACHE
    SCB_CleanInvalidateDCache_by_Addr((uint32_t*)graphics.params.user, sizeof(graphics.params.user));
#endif
    encoder.debounce();
    enc_data[0] = int16_t(
      encoder.isRisingEdge() | (encoder.isFallingEdge() << 1) | (encoder.isLongPress() << 2));
    enc_data[1] = int16_t(encoder.getValue());
    graphics.params.updateEncoders(enc_data, 2);

    for(int i = NOF_ADC_VALUES; i < NOF_PARAMETERS; ++i) {
      graphics.params.updateValue(i, 0);
    }

    graphics.draw();
    graphics.display();
  }
*/
  
  owl.loop();
}
