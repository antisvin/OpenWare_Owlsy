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
// #include "errorhandlers.h"
#include "message.h"
#include "gpio.h"
#include "qspicontrol.h"
#include "ProgramManager.h"
#include "PatchRegistry.h"
#include "OpenWareMidiControl.h"
#include "SoftwareEncoder.hpp"
#include "SoftwareLed.hpp"
#include "TakeoverControls.h"
#include "VersionToken.h"
#include "rainbow.h"

TakeoverControls<40, int16_t> takeover;
volatile uint8_t patchselect;
static uint8_t parameter_offset;
uint16_t enc_value;
// #define MAX_B1_PRESS (2000 / MAIN_LOOP_SLEEP_MS)

extern int16_t parameter_values[NOF_PARAMETERS];
static DebouncedButton b1(SW1_GPIO_Port, SW1_Pin);
static DebouncedButton b2(SW2_GPIO_Port, SW2_Pin);

static RGBLed led1(LedChannel(Pin(LED1_R_GPIO_Port, LED1_R_Pin), true),
    LedChannel(Pin(LED1_G_GPIO_Port, LED1_G_Pin), true),
    LedChannel(Pin(LED1_B_GPIO_Port, LED1_B_Pin), true));
static RGBLed led2(LedChannel(Pin(LED2_R_GPIO_Port, LED2_R_Pin), true),
    LedChannel(Pin(LED2_G_GPIO_Port, LED2_G_Pin), true),
    LedChannel(Pin(LED2_B_GPIO_Port, LED2_B_Pin), true));

static SoftwareEncoder encoder(ENC_A_GPIO_Port, ENC_A_Pin, ENC_B_GPIO_Port,
    ENC_B_Pin, ENC_SW_GPIO_Port, ENC_SW_Pin);

enum Colour {
    COL_RED = 0xFF0000,
    COL_ORANGE = 0xFF8000,
    COL_YELLOW = 0xFFFF00,
    COL_GREEN = 0x00FF00,
    COL_CYAN = 0x00FFFF,
    COL_BLUE = 0x0000FF,
    COL_PURPLE = 0x8000FF,
    COL_WHITE = 0xFFFFFF,
};

static Colour colours[] = {
    COL_RED,
    COL_ORANGE,
    COL_YELLOW,
    COL_GREEN,
    COL_CYAN,
    COL_BLUE,
    COL_PURPLE,
    COL_WHITE,
};

#define PATCH_RESET_COUNTER (768 / MAIN_LOOP_SLEEP_MS)
#define PATCH_CONFIG_COUNTER (3072 / MAIN_LOOP_SLEEP_MS)
#define PATCH_INDICATOR_COUNTER (256 / MAIN_LOOP_SLEEP_MS)
#define PATCH_CONFIG_KNOB_THRESHOLD (4096 / 8)
#define PATCH_CONFIG_PROGRAM_CONTROL 0
// #define PATCH_CONFIG_VOLUME_CONTROL 3

bool patchSelection;

int16_t getParameterValue(uint8_t pid) {
    if (pid < NOF_PARAMETERS)
        return takeover.get(pid);
    else
        return 0;
}

// called from program, MIDI, or (potentially) digital bus
void setParameterValue(uint8_t pid, int16_t value) {
    if (pid < NOF_PARAMETERS) {
        takeover.set(pid, value);
        takeover.reset(pid, false);
    }
    // TODO: which color? use rainbow gradient?
    //    if (pid == LOAD_INDICATOR_PARAMETER && owl.getOperationMode() == LOAD_MODE)
    //        setLed(0, value);
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
        // smooth_adc_values[i] = __SSAT(
        //   (smooth_adc_values[i] * 3 + 4095 + 121 - 2 * adc_values[i]) >> 2, 13);
        // i++;
        //  Knobs are unipolar
        smooth_adc_values[i] = __USAT(
            (smooth_adc_values[i] * 3 + 4095 + 132 - 2 * adc_values[i]) >> 2, 12);
        // i++;
    }
}
}

void updateParameters(int16_t* parameter_values, size_t parameter_len,
    uint16_t* adc_values, size_t adc_len) {
    // cv inputs are ADC_A, C, E, G
    // knobs are ADC_B, D, F, H
    if (owl.getOperationMode() == RUN_MODE) {
        for (size_t i = 0; i < NOF_ADC_VALUES; ++i) {
            takeover.update(parameter_offset + i, smooth_adc_values[i], 31);
        }
    }
    /*
    for (size_t i = 0; i < 2; ++i) {
        int16_t x = takeover.get(i);
        int16_t cv = smooth_adc_values[i]
            // getAttenuatedCV(i, smooth_adc_values);
            parameter_values[i] = __USAT(x + cv, 12);
    }
    */
}

void onChangePin(uint16_t pin) {
    switch (pin) {
    case SW1_Pin: {
        bool state = // HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET;
            b1.isRisingEdge() ||
            HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET;
        setButtonValue(BUTTON_A, state);
        setButtonValue(PUSHBUTTON, state);
        break;
    }
    case SW2_Pin: {
        bool state = HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET;
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

    patchselect = program.getProgramIndex();
}

// uint16_t progress = 0;
// void setProgress(uint16_t value, const char* msg) {
//     progress = value;
// value == 4095 ? 0 : value * 6; // TODO?
//}

void setLed(uint8_t led, uint32_t rgb) {
    if (led == 0) {
        led1.set(rgb);
    }
    else if (led == 1) {
        led2.set(rgb);
    }
}

uint32_t getLed(uint8_t led) {
    if (led == 0) {
        return led1.get();
    }
    else if (led == 1) {
        return led2.get();
    }
    else {
        return 0;
    }
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
        if (encoder.isRisingEdge()) {
            owl.setOperationMode(CONFIGURE_MODE);
        }
        break;
    case LOAD_MODE: {
        float hue = float(getParameterValue(LOAD_INDICATOR_PARAMETER)) / 4095;
        setLed(0, RGBLed::fromHSB(hue, 1.0, 1.0));
        setLed(1, RGBLed::fromHSB(hue, 1.0, 1.0));
        if (getErrorStatus() != NO_ERROR)
            owl.setOperationMode(ERROR_MODE);
        break;
    }
    case RUN_MODE:
        if (getErrorStatus() != NO_ERROR) {
            owl.setOperationMode(ERROR_MODE);
        }
        else {
            // counter = 0;
            setLed(0, getButtonValue(BUTTON_A) ? 0xff00 : 0);
            setLed(1, getButtonValue(BUTTON_B) ? 0xff00 : 0);
        }
        if (encoder.isRisingEdge()) {
            owl.setOperationMode(CONFIGURE_MODE);
        }

        break;
    /*
    case CONFIGURE_MODE:
        /// XXX
        if (patchSelection) {
            if (isModeButtonPressed()) {
                // update patch control
                int16_t value = smooth_adc_values[1];
                // 4095 - 2 * getAnalogValue(1);

                if (abs(takeover.get(8) - value) >= PATCH_CONFIG_KNOB_THRESHOLD)
    { takeover.set(8, value);

                    float pos = 0.5f +
                        (value * (registry.getNumberOfPatches() - 1)) / 4096.0f;
                    uint8_t idx = roundf(pos);
                    if (abs(patchselect - pos) > 0.6 && registry.hasPatch(idx))
    { // ensure a small dead zone toggleLed(0); patchselect = idx;
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
                owl.setOperationMode(RUN_MODE);
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
                }
            }
            else {
                owl.setOperationMode(RUN_MODE);
            }
        }
        //            takeover.reset(false);

        break;
    */
    case ERROR_MODE:
        setLed(0, counter > PATCH_RESET_COUNTER * 0.5 ? 0xff0000 : 0);
        setLed(1, counter <= PATCH_RESET_COUNTER * 0.5 ? 0xff0000 : 0);
        /*
        if (isModeButtonPressed()) {
            setErrorStatus(NO_ERROR);
            owl.setOperationMode(CONFIGURE_MODE_ENTERED);
        }
        */
        if (encoder.isRisingEdge()) {
            owl.setOperationMode(CONFIGURE_MODE);
        }
        break;
        /*
    case PARAM_MODE: {
        uint8_t group = parameter_offset % 3;
        uint8_t led_colour = parameter_offset / 3;
        bool show_led2 = group == 0 || group == 2;
        bool show_led1 = group == 1 || group == 2;

        if (counter & 0x100) {
            if (show_led1) {
                // LED1 on
            }
            else {
                // LED1 off
            }
            if (show_led2) {
                // LED2 on
            }
            else {
                // LED2 off
            }
        }
        else {
            // Both LEDs off
        }
        break;
    }
*/    
    case CONFIGURE_MODE:
        if (counter < PATCH_RESET_COUNTER / 2 && registry.hasPatch(patchselect)) {
            uint8_t bank = patchselect / 8;
            uint8_t slot = patchselect - bank * 8;
            setLed(0, (uint32_t)colours[bank]);
            setLed(1, (uint32_t)colours[slot]);
        }
        else {
            setLed(0, 0);
            setLed(1, 0);
        }
        if (encoder.isFallingEdge()) {
            setLed(0, 0);
            setLed(1, 0);
            if (registry.hasPatch(patchselect)) {
                program.loadProgram(patchselect);
                program.resetProgram(false);
            }
            else
                owl.setOperationMode(RUN_MODE);
        }
        else {
            uint16_t new_enc_value = encoder.getValue();
            if (new_enc_value > enc_value) {
                patchselect++;
                if (patchselect >= registry.getNumberOfPatches())
                    patchselect = 0;
                enc_value = new_enc_value;
            }
            else if (new_enc_value < enc_value) {
                patchselect--;
                if (patchselect >= registry.getNumberOfPatches()) {
                    patchselect = registry.getNumberOfPatches() - 1;
                }
                enc_value = new_enc_value;
            }
        }
    }
    if (++counter >= PATCH_RESET_COUNTER)
        counter = 0;
}

void onChangeMode(OperationMode new_mode, OperationMode old_mode) {
    counter = 0;
    enc_value = encoder.getValue();
    patchselect = settings.program_index;
    setLed(0, 0);
    setLed(1, 0);
    /*
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
        setLed(0, getParameterValue(LOAD_INDICATOR_PARAMETER)); // restore to saved LED state
    }
    */
}

void loop() {
    encoder.updateCounter();
    encoder.debounce();
    b1.debounce();
    if (b1.isChanged()) {
        onChangePin(SW1_Pin);
    }
    b2.debounce();
    if (b2.isChanged()) {
        onChangePin(SW2_Pin);
    }
    owl.loop();
    /*
    static bool sw4_state = false;
    if (sw4_state != !sw4.get())
        sw4_state = updatePin(4, sw4);
    */
    updatePreset();
    led1.update();
    led2.update();
}
