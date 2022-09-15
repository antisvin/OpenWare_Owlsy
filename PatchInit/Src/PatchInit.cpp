#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "device.h"
#include "ApplicationSettings.h"
#include "Codec.h"
#include "Owl.h"
#include "DebouncedButton.hpp"
#include "MidiController.h"
//#include "errorhandlers.h"
#include "message.h"
#include "gpio.h"
#include "qspicontrol.h"
#include "ProgramManager.h"
#include "PatchRegistry.h"
#include "OpenWareMidiControl.h"
#include "TakeoverControls.h"
#include "VersionToken.h"

TakeoverControls<9, int16_t> takeover;
volatile uint8_t patchselect;
volatile bool b1_pressed;
#define MAX_B1_PRESS (2000 / MAIN_LOOP_SLEEP_MS)
uint16_t b1_counter;
extern int16_t parameter_values[NOF_PARAMETERS];
static DebouncedButton b1(SW1_GPIO_Port, SW1_Pin);
static uint32_t led_level;

// 12x12 bit multiplication with unsigned operands and result
#define U12_MUL_U12(a, b) (__USAT(((uint32_t)(a) * (b)) >> 12, 12))

#define CV_ATTENUATION_DEFAULT 2186 // calibrated to provide 1V/oct over 5V
// TODO

#define PATCH_RESET_COUNTER (768 / MAIN_LOOP_SLEEP_MS)
#define PATCH_CONFIG_COUNTER (3072 / MAIN_LOOP_SLEEP_MS)
#define PATCH_INDICATOR_COUNTER (256 / MAIN_LOOP_SLEEP_MS)
#define PATCH_CONFIG_KNOB_THRESHOLD (4096 / 8)
#define PATCH_CONFIG_PROGRAM_CONTROL 0
//#define PATCH_CONFIG_VOLUME_CONTROL 3

bool patchSelection;

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
        if (owl.getOperationMode() == RUN_MODE) {
            HAL_DAC_SetValue(
                &DAC_PERIPH, DAC_CHANNEL_2, DAC_ALIGN_12B_R, __USAT(value, 12));
        }
        else {
            HAL_DAC_SetValue(&DAC_PERIPH, DAC_CHANNEL_2, DAC_ALIGN_12B_R, led_level);
        };
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
        // IIR exponential filter with lambda 0.75: y[n] = 0.75*y[n-1] +
        // 0.25*x[n] We include offset to match 0 point too, but it may differ
        // between devices CV is bipolar
        smooth_adc_values[i] = __SSAT(
            (smooth_adc_values[i] * 3 + 4095 + 121 - 2 * adc_values[i]) >> 2, 13);
        i++;
        // Knobs are unipolar
        smooth_adc_values[i] = __USAT(
            (smooth_adc_values[i] * 3 + 4095 + 132 - 2 * adc_values[i]) >> 2, 12);
        i++;
    }
    // tr_out_a_pin.toggle();
}
}

void updateParameters(int16_t* parameter_values, size_t parameter_len,
    uint16_t* adc_values, size_t adc_len) {
    // cv inputs are ADC_A, C, E, G
    // knobs are ADC_B, D, F, H
    if (owl.getOperationMode() == RUN_MODE) {
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

uint16_t progress = 0;
void setProgress(uint16_t value, const char* msg) {
    // debugMessage(msg, (int)(100*value/4095));
    progress = value == 4095 ? 0 : value * 6;
}

void setLed(uint8_t led, uint32_t rgb) {
    setAnalogValue(0, rgb);
    led_level = rgb;
}

uint32_t getLed(uint8_t led) {
    return led_level;
}

void toggleLed(uint8_t led) {
    setLed(led, getLed(led) == 0 ? 4095 : 0);
}

static uint32_t counter = 0;
static void updatePreset() {
    switch (owl.getOperationMode()) {
    case STARTUP_MODE:
    case STREAM_MODE:
        setLed(0, counter > PATCH_RESET_COUNTER / 2 ? 4095 : 0);
        if (counter-- == 0)
            counter = PATCH_RESET_COUNTER;
        break;
    case LOAD_MODE: {
        uint16_t value = progress;
        setLed(0, value);
        // if (value == 0)
        //     value = counter * 4095 * 6 / PATCH_RESET_COUNTER;
        if (getErrorStatus() != NO_ERROR || isModeButtonPressed())
            owl.setOperationMode(ERROR_MODE);
        break;
    }
    case RUN_MODE:
        if (isModeButtonPressed()) {
            owl.setOperationMode(CONFIGURE_MODE_ENTERED);
        }
        else if (getErrorStatus() != NO_ERROR) {
            owl.setOperationMode(ERROR_MODE);
        }
        else {
            counter = 0;
        }
        break;
    case CONFIGURE_MODE_ENTERED:
        if (counter % 128 == 0) {
            toggleLed(0);
        }
        if (counter == PATCH_RESET_COUNTER - 1)
            owl.setOperationMode(CONFIGURE_MODE);
        break;
    case CONFIGURE_MODE:
        /// XXX
        if (patchSelection) {
            if (isModeButtonPressed()) {
                // update patch control
                int16_t value = getAnalogValue(PATCH_CONFIG_PROGRAM_CONTROL);
                if (abs(takeover.get(8) - value) >= PATCH_CONFIG_KNOB_THRESHOLD) {
                    takeover.set(8, value);

                    float pos = 0.5f +
                        (value * (registry.getNumberOfPatches() - 1)) / 4096.0f;
                    uint8_t idx = roundf(pos);
                    if (abs(patchselect - pos) > 0.6 && registry.hasPatch(idx)) { // ensure a small dead zone
                        toggleLed(0);
                        patchselect = idx;
                        midi_tx.sendPc(patchselect);
                        // update patch indicator
                        // counter = PATCH_RESET_COUNTER +
                        //    PATCH_INDICATOR_COUNTER * (2 * patchselect + 1);
                    }
                }
            }
            else {
                if (program.getProgramIndex() != patchselect &&
                    registry.hasPatch(patchselect)) {
                    program.loadProgram(
                        patchselect); // enters load mode (calls onChangeMode)
                    program.resetProgram(false);
                }
                else {
                    owl.setOperationMode(RUN_MODE);
                }
            }
        }
        else {
            // Attenuation
            if (isModeButtonPressed()) {
                // if (isModeButtonPressed()) {
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
                owl.setOperationMode(RUN_MODE);
            }
        }
        //            takeover.reset(false);

        break;
    case ERROR_MODE:
        // setLed(1, counter > PATCH_RESET_COUNTER * 0.5 ? 4095 : 0);
        if (isModeButtonPressed()) {
            setErrorStatus(NO_ERROR);
            owl.setOperationMode(CONFIGURE_MODE_ENTERED);
        }
        break;
    }
    if (++counter >= PATCH_RESET_COUNTER)
        counter = 0;
}

void onChangeMode(OperationMode new_mode, OperationMode old_mode) {
    counter = 0;
    // static uint32_t saved_led = NO_COLOUR;
    // if (old_mode == RUN_MODE) {
    //     saved_led = getParameterValue(PARAMETER)); // leaving RUN_MODE, save LED state
    // }
    setLed(0, 0);
    debugMessage("MODE =", (int)new_mode);
    if (new_mode == CONFIGURE_MODE) {
        if (HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET) {
            // Switch is on, patch selection mode
            patchSelection = true;
            patchselect = program.getProgramIndex();
            takeover.set(8, getAnalogValue(PATCH_CONFIG_PROGRAM_CONTROL));
        }
        else {
            // Switch is off, attenuation configuration
            patchSelection = false;
        }
    }
    else if (new_mode == RUN_MODE) {
        setLed(0, takeover.get(LOAD_INDICATOR_PARAMETER)); // restore to saved LED state
    }
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
    updatePreset();
}
