#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "device.h"
#include "Owl.h"
#include "Graphics.h"
#include "message.h"
#include "gpio.h"
#include "qspicontrol.h"
#include "ProgramManager.h"
#include "PatchRegistry.h"
#include "OpenWareMidiControl.h"
#include "SoftwareEncoder.hpp"
#include "TakeoverControls.h"
#include "VersionToken.h"

extern bool updateUi;
extern uint16_t button_values;

Graphics graphics;
Browser sd_browser SD_BUF;

static SoftwareEncoder encoder(
  ENC_A_GPIO_Port, ENC_A_Pin,
  ENC_B_GPIO_Port, ENC_B_Pin, 
  ENC_CLICK_GPIO_Port, ENC_CLICK_Pin);

TakeoverControls<2, uint16_t> takeover;

extern "C" void updateEncoderCounter(){
  encoder.updateCounter();
}

int16_t getParameterValue(uint8_t pid) {
    if (pid < NOF_PARAMETERS) {
      if (pid < NOF_ADC_VALUES / 2) {
        return takeover.get(pid);
      }
      else {
        return graphics.params.getValue(pid);
      }
    }
    else
    {
        return 0;      
    }
}

// called from program, MIDI, or (potentially) digital bus
void setParameterValue(uint8_t pid, int16_t value) {
    if (pid < NOF_PARAMETERS) {
      if (pid < NOF_ADC_VALUES / 2) {
        takeover.set(pid, value);
        takeover.reset(pid, false);
        graphics.params.setValue(pid, takeover.get(pid));
      }
      else  { 
        graphics.params.setValue(pid, value);
      }
    }
}


static uint16_t smooth_adc_values[NOF_ADC_VALUES];
extern "C" {
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    // this runs at apprx 3.3kHz
    // with 64.5 cycles sample time, 30 MHz ADC clock, and ClockPrescaler = 32
    extern uint16_t adc_values[NOF_ADC_VALUES];
    for (size_t i = 0; i < NOF_ADC_VALUES; i++) {
        // IIR exponential filter with lambda 0.75: y[n] = 0.75*y[n-1] + 0.25*x[n]
        smooth_adc_values[i] = (smooth_adc_values[i]*3 + adc_values[i]) >> 2;
    }
}
}

void updateParameters(int16_t* parameter_values, size_t parameter_len, uint16_t* adc_values, size_t adc_len){
#ifdef USE_CACHE
  //SCB_InvalidateDCache_by_Addr((uint32_t*)adc_values, sizeof(adc_values));
#endif
  for (int i = 0; i < NOF_ADC_VALUES / 2; i++) {
    // CV ADC channels are inverted
    takeover.update(i, smooth_adc_values[i * 2] + 4095 - smooth_adc_values[i * 2 + 1] * 2, 31);
    graphics.params.updateValue(i, takeover.get(i));
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

  extern I2C_HandleTypeDef OLED_I2C;
  graphics.begin(&OLED_I2C);

  owl.setup();
}

static int16_t enc_data[2];
extern uint16_t adc_values[];

void loop(){
  updateEncoderCounter();
  if (updateUi){
#if defined USE_CACHE
//    SCB_CleanInvalidateDCache_by_Addr((uint32_t*)graphics.params.user, sizeof(graphics.params.user));
#endif
    encoder.debounce();
    enc_data[0] = int16_t(
      encoder.isRisingEdge() | (encoder.isFallingEdge() << 1) | (encoder.isLongPress() << 2));
    enc_data[1] = int16_t(encoder.getValue());

    graphics.params.updateEncoders(enc_data, 2);

    //for(int i = NOF_ADC_VALUES / 2; i < NOF_PARAMETERS; ++i) {
    //  graphics.params.updateValue(i, 0);
    //}

    graphics.draw();
    graphics.display();
  }
  
  owl.loop();
}
