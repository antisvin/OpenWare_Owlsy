#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "device.h"
#include "ApplicationSettings.h"
#include "Codec.h"
#include "Owl.h"
#include "Pin.h"
#include "DebouncedButton.hpp"
//#include "errorhandlers.h"
#include "message.h"
#include "gpio.h"
#include "qspicontrol.h"
#include "ProgramManager.h"
#include "PatchRegistry.h"
#include "OpenWareMidiControl.h"
#include "TakeoverControls.h"
#include "VersionToken.h"

Pin sw1(SW1_GPIO_Port, SW1_Pin);
Pin sw2(SW2_GPIO_Port, SW2_Pin);
// Pin sw3(SW3_GPIO_Port, SW3_Pin);
// Pin sw4(SW4_GPIO_Port, SW4_Pin);

TakeoverControls<8, int16_t> takeover;
volatile uint8_t patchselect;
volatile bool b1_pressed;
#define MAX_B1_PRESS 2000
uint16_t b1_counter;
extern int16_t parameter_values[NOF_PARAMETERS];
static DebouncedButton b1(SW1_GPIO_Port, SW1_Pin);

// 12x12 bit multiplication with unsigned operands and result
#define U12_MUL_U12(a, b) (__USAT(((uint32_t)(a) * (b)) >> 12, 12))

#define CV_ATTENUATION_DEFAULT 2186 // calibrated to provide 1V/oct over 5V
// TODO

int16_t getParameterValue(uint8_t pid) {
    if (pid < 4)
        return takeover.get(pid);
    else if (pid < NOF_PARAMETERS)
        return parameter_values[pid];
    return 0;
}

// called from program, MIDI, or (potentially) digital bus
void setParameterValue(uint8_t pid, int16_t value) {
    if (pid < 4) {
        takeover.set(pid, value);
        takeover.reset(pid, false);
    }
    else if (pid < NOF_PARAMETERS) {
        parameter_values[pid] = value;
    }
}

void setGateValue(uint8_t bid, int16_t value) {
    if (bid == BUTTON_A || bid == PUSHBUTTON) {
        HAL_GPIO_WritePin(GATE_OUT1_GPIO_Port, GATE_OUT1_Pin,
            value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else if (bid == BUTTON_B) {
        HAL_GPIO_WritePin(GATE_OUT2_GPIO_Port, GATE_OUT2_Pin,
            value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void setAnalogValue(uint8_t ch, int16_t value) {
    extern DAC_HandleTypeDef DAC_PERIPH;
    switch (ch) {
    case PARAMETER_F:
        HAL_DAC_SetValue(
            &DAC_PERIPH, DAC_CHANNEL_1, DAC_ALIGN_12B_R, __USAT(value, 12));
        break;
    case PARAMETER_G:
        HAL_DAC_SetValue(
            &DAC_PERIPH, DAC_CHANNEL_2, DAC_ALIGN_12B_R, __USAT(value, 12));
        break;
    }
}

bool isModeButtonPressed() {
    return b1_counter >= MAX_B1_PRESS;
    // return !sw5.get(); // HAL_GPIO_ReadPin(SW5_GPIO_Port, SW5_Pin) == GPIO_PIN_RESET;
}

int16_t getAttenuatedCV(uint8_t index, int16_t* adc_values) {
    // 12x12 bit multiplication with signed operands and no saturation
    return ((int32_t)adc_values[index * 2] * takeover.get(index + 4)) >> 12;
}

static int16_t smooth_adc_values[NOF_ADC_VALUES];
extern "C" {
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    // this runs at apprx 3.3kHz
    // with 64.5 cycles sample time, 30 MHz ADC clock, and ClockPrescaler = 32
    extern uint16_t adc_values[NOF_ADC_VALUES];
    for (size_t i = 0; i < NOF_ADC_VALUES; i++) {
        // IIR exponential filter with lambda 0.75: y[n] = 0.75*y[n-1] + 0.25*x[n]
        // We include offset to match 0 point too, but it may differ between devices
        // CV is bipolar
        smooth_adc_values[i] =
            __SSAT((smooth_adc_values[i] * 3 + 4095 + 121 - 2 * adc_values[i]) >> 2, 13);
        i++;
        // Knobs are unipolar
        smooth_adc_values[i] =
            __USAT((smooth_adc_values[i] * 3 + 4095 + 132 - 2 * adc_values[i]) >> 2, 12);
        i++;
    }
    // tr_out_a_pin.toggle();
}
}

void updateParameters(int16_t* parameter_values, size_t parameter_len,
    uint16_t* adc_values, size_t adc_len) {
    // cv inputs are ADC_A, C, E, G
    // knobs are ADC_B, D, F, H
    if (isModeButtonPressed()) {
        for (size_t i = 0; i < 4; ++i) {
            int32_t value = smooth_adc_values[i * 2 + 1] - 2048;
            // attenuvert from -2x to +2x
            if (value > 0) {
                value = (value * value) >> 9;
            }
            else {
                value = -(value * value) >> 9;
            }
            takeover.update(i + 4, value, 31);
            // if (takeover.taken(i + 4))
            //     setLed(i + 1, 0);
            // else
            //     setLed(i + 1, 4095);
        }
    }
    else {
        for (size_t i = 0; i < 4; ++i) {
            takeover.update(i, smooth_adc_values[i * 2 + 1], 31);
            //            if (takeover.taken(i))
            //                setLed(i + 1, abs(getAttenuatedCV(i, smooth_adc_values)));
            //            else
            //                setLed(i + 1, 4095);
        }
        for (size_t i = 0; i < 4; ++i) {
            int16_t x = takeover.get(i);
            int16_t cv = getAttenuatedCV(i, smooth_adc_values);
            parameter_values[i] = __USAT(x + cv, 12);
        }
        parameter_values[4] = __USAT(takeover.get(4), 12);
    }
}

void onChangePin(uint16_t pin) {
    switch (pin) {
    case SW1_Pin:
    case GATE_IN1_Pin: {
        bool state = // HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET;
            b1.isRisingEdge() ||
            HAL_GPIO_ReadPin(GATE_IN1_GPIO_Port, GATE_IN1_Pin) == GPIO_PIN_RESET;
        b1_pressed = state;
        if (!state)
            b1_counter = 0;
        setButtonValue(BUTTON_A, state);
        setButtonValue(PUSHBUTTON, state);
        break;
    }
    case SW2_Pin:
    case GATE_IN2_Pin: {
        bool state = HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET ||
            HAL_GPIO_ReadPin(GATE_IN2_GPIO_Port, GATE_IN2_Pin) == GPIO_PIN_RESET;
        setButtonValue(BUTTON_B, state);
        break;
    }
    }
}

void setup() {
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

    HAL_GPIO_WritePin(GATE_OUT1_GPIO_Port, GATE_OUT1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GATE_OUT2_GPIO_Port, GATE_OUT2_Pin, GPIO_PIN_SET);
    for (size_t i = 4; i < 8; ++i) {
        takeover.set(i, CV_ATTENUATION_DEFAULT);
        takeover.reset(i, false);
    }
    patchselect = program.getProgramIndex();
}

#define PATCH_RESET_COUNTER (600 / MAIN_LOOP_SLEEP_MS)
uint16_t progress = 0;
void setProgress(uint16_t value, const char* msg) {
    // debugMessage(msg, (int)(100*value/4095));
    progress = value == 4095 ? 0 : value * 6;
}

static uint32_t counter = 0;
static void update_preset() {
    switch (owl.getOperationMode()) {
    case STARTUP_MODE:
    case STREAM_MODE:
    case LOAD_MODE: {
        uint16_t value = progress;
        if (value == 0)
            value = counter * 4095 * 6 / PATCH_RESET_COUNTER;
        if (getErrorStatus() != NO_ERROR || isModeButtonPressed())
            owl.setOperationMode(ERROR_MODE);
        break;
    }
    case RUN_MODE:
        if (isModeButtonPressed()) {
            owl.setOperationMode(CONFIGURE_MODE);
        }
        else if (getErrorStatus() != NO_ERROR) {
            owl.setOperationMode(ERROR_MODE);
        }
        break;
    case CONFIGURE_MODE:
        /// XXX
        if (isModeButtonPressed()) {
            for (int i = 1; i <= 4; ++i) {
                uint32_t colour = NO_COLOUR;
                if (patchselect == i)
                    colour = YELLOW_COLOUR;
                else if (patchselect == i + 4)
                    colour = RED_COLOUR;
                setLed(6 + i, colour);
            }
            if (takeover.taken(9)) {
                uint8_t value = (takeover.get(9) >> 6) + 63;
                if (settings.audio_output_gain != value) {
                    settings.audio_output_gain = value;
                    codec.setOutputGain(value);
                }
            }
        }
        else {
            if (program.getProgramIndex() != patchselect &&
                registry.getPatchBlock(patchselect) != NULL) {
                // change patch on mode button release
                program.loadProgram(patchselect); // enters load mode (calls onChangeMode)
                program.resetProgram(false);
            }
            else {
                owl.setOperationMode(RUN_MODE);
            }
            takeover.reset(false);
        }
        break;
    case ERROR_MODE:
        // setLed(1, counter > PATCH_RESET_COUNTER * 0.5 ? 4095 : 0);
        if (isModeButtonPressed()) {
            setErrorStatus(NO_ERROR);
            owl.setOperationMode(CONFIGURE_MODE);
        }
        break;
    }
    if (++counter >= PATCH_RESET_COUNTER)
        counter = 0;
}

void loop() {
    b1.debounce();
    if (b1.isChanged()) {
        onChangePin(SW1_Pin);
    }
    if (b1_pressed && b1_counter < MAX_B1_PRESS) {
        b1_counter++;
    }
    owl.loop();
    /*
    static bool sw4_state = false;
    if (sw4_state != !sw4.get())
        sw4_state = updatePin(4, sw4);
    */
    update_preset();
}
