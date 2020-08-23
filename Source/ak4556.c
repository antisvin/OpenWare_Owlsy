#include "ak4556.h"
#include "gpio.h"
#include "device.h"


// Yeah this codec's implementation is brain dead, but nothing else is exposed to us

void codec_init(){
    clearPin(CODEC_RESET1_GPIO_Port, CODEC_RESET1_Pin);
#ifdef DUAL_CODEC
    clearPin(CODEC_RESET2_GPIO_Port, CODEC_RESET2_Pin);
#endif
    // Datasheet specifies minimum 150ns
    HAL_Delay(1);
    setPin(CODEC_RESET1_GPIO_Port, CODEC_RESET1_Pin);
#ifdef DUAL_CODEC
    setPin(CODEC_RESET2_GPIO_Port, CODEC_RESET2_Pin);
#endif
}

void codec_bypass(int bypass){
    // Not supported
}

void codec_mute(bool mute){  
    // Not supported
}

void codec_set_gain_in(int8_t level){
    // Not supported
}

void codec_set_gain_out(int8_t level){
    // Not supported
}