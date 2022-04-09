#include "Owl.h"
#include "gpio.h"
#include "Graphics.h"
#include "Encoders.h"

#ifdef USE_SCREEN
#include "PreenFM3ParameterController.hpp"
#endif

static uint32_t nextDrawTime;

extern "C" {
void setPortMode(uint8_t index, uint8_t mode) {
}
uint8_t getPortMode(uint8_t index) {
    return 0;
}
}

#ifdef USE_SCREEN
Graphics graphics GRAPHICS_RAM;
PreenFM3ParameterController params;

void defaultDrawCallback(uint8_t* pixels, uint16_t width, uint16_t height) {
}
#endif

extern TIM_HandleTypeDef htim1;
Encoders encoders;

void setProgress(uint16_t value, const char* reason){
    params.setProgress(value, reason);
}

extern bool pushToTftInProgress;

void onSetup() {
    encoders.insertListener(&params);

#ifdef USE_SCREEN
    setPin(LED_TEST_GPIO_Port, LED_TEST_Pin);
    setPin(LED_CONTROL_GPIO_Port, LED_CONTROL_Pin);
    extern SPI_HandleTypeDef OLED_SPI;
    graphics.begin(&params, &OLED_SPI);
    //  clearPin(BACKLIGHT1_GPIO_Port, BACKLIGHT1_Pin);
    TIM1->CCR2 = 20;
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
#endif

    nextDrawTime = HAL_GetTick();
}

void updateEncoders() {
    encoders.checkStatus(1, 1);
    encoders.checkSimpleStatus();
    encoders.processActions();
}

EraseStorageTask erase_task;


#if defined USE_SCREEN
void runScreenTask(void* p){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = SCREEN_LOOP_SLEEP_MS / portTICK_PERIOD_MS;
    uint32_t prev_fps_update = 0;
    uint16_t num_frames = 0;
    uint32_t tick;
    for(;;){
        vTaskDelayUntil(&xLastWakeTime, xFrequency);    
        onScreenDraw();
        tick = HAL_GetTick();
    
        // FPS counter, recalculated every 500 frames
        if (tick - prev_fps_update > 500) {
            params.current_state.fps = num_frames * 1000 / (tick - prev_fps_update);
            num_frames = 0;
            prev_fps_update = tick;
        }

        num_frames++;
        params.updateState();

        while (params.dirty) {
            if (pushToTftInProgress) {
                vTaskDelay(1);
            }
            else {
                graphics.draw();
                graphics.display();
            }
        }
    }
}
#endif

void onLoop(void) {
#ifdef USE_SCREEN
    updateEncoders();
#endif /* USE_SCREEN */
}
