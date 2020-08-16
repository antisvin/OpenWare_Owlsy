#include "ak4556.h"
#include "gpio.h"
#include "device.h"


// Yeah this codec is implementation is brain dead, but nothing else is exposed to us

void codec_init(){
    clearPin(CODEC_RESET_GPIO_Port, CODEC_RESET_Pin);
    // Datasheet specifies minimum 150ns
    HAL_Delay(1);
    setPin(CODEC_RESET_GPIO_Port, CODEC_RESET_Pin);
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