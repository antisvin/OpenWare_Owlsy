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
#include "VersionToken.h"
#include "SdCardTask.hpp"

extern bool updateUi;
extern uint16_t button_values;

Graphics graphics;

static SoftwareEncoder encoder(
  ENC_A_GPIO_Port, ENC_A_Pin,
  ENC_B_GPIO_Port, ENC_B_Pin, 
  ENC_CLICK_GPIO_Port, ENC_CLICK_Pin);

extern "C" void updateEncoderCounter(){
  encoder.updateCounter();
}

void setGateValue(uint8_t bid, int16_t value){
  if (bid == BUTTON_C || bid == PUSHBUTTON) {
    HAL_GPIO_WritePin(GATE_OUT_GPIO_Port, GATE_OUT_Pin, value ? GPIO_PIN_SET :  GPIO_PIN_RESET);
    // We have to update button values here, because currently it's not propagated from program vector. A bug?
    // setButtonValue(BUTTON_C, value);
    //button_values &= ~((!value) << BUTTON_C);
    //button_values |= (bool(value) << BUTTON_C);
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
  graphics.params.updateValue(0, 4095 - adc_values[0]);
  graphics.params.updateValue(1, 4095 - adc_values[1]);
  graphics.params.updateValue(2, 4095 - adc_values[2]);
  graphics.params.updateValue(3, 4095 - adc_values[3]);
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

SdCardTask sd_card_task;
uint8_t buf[4];

void setup(){
  HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET); // OLED off

  qspi_init(QSPI_MODE_MEMORY_MAPPED);
  
  /* This doesn't work with bootloader, need to find how to deinit it earlier*/
  #if 0
  if (qspi_init(QSPI_MODE_MEMORY_MAPPED) != MEMORY_OK){
    // We can end here only if QSPI settings are misconfigured
    error(RUNTIME_ERROR, "Flash init error");
  }
  #endif

  extern SPI_HandleTypeDef OLED_SPI;
  graphics.begin(&OLED_SPI);

  owl.setup();
  setButtonValue(PUSHBUTTON, 0); 

  //BSP_SD_Init();
  sd_card_task.setDestination("bank05.wav", buf, 4);
  owl.setBackgroundTask(&sd_card_task);
}

static int16_t enc_data[2];

void loop(){
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
  
  owl.loop();
}
