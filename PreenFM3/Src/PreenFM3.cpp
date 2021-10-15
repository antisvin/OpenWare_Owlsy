#include "Owl.h"
#include "gpio.h"
#include "Graphics.h"
#include "Encoders.h"

static uint32_t nextDrawTime;

extern "C" {
#if 0
  void eeprom_unlock(){}
  int eeprom_write_block(uint32_t address, void* data, uint32_t size)
  {return 0;}
  int eeprom_write_word(uint32_t address, uint32_t data)
  {return 0;}
  int eeprom_write_byte(uint32_t address, uint8_t data)
  {return 0;}
  int eeprom_erase(uint32_t address)
  {return 0;}
  int eeprom_wait()
  {return 0;}
  int eeprom_erase_sector(uint32_t sector)
  {return 0;}
  int eeprom_write_unlock(uint32_t wrp_sectors)
  {return 0;}
  int eeprom_write_lock(uint32_t wrp_sectors)
  {return 0;}
  uint32_t eeprom_write_protection(uint32_t wrp_sectors)
  {return 0;}
#endif
void setPortMode(uint8_t index, uint8_t mode) {
}
uint8_t getPortMode(uint8_t index) {
    return 0;
}
}

// extern TIM_HandleTypeDef ENCODER_TIM1;
// extern TIM_HandleTypeDef ENCODER_TIM2;

#ifdef USE_SCREEN
Graphics graphics GRAPHICS_RAM;

void pfmDefaultDrawCallback(uint8_t* pixels, uint16_t width, uint16_t height) {
}
#endif

extern TIM_HandleTypeDef htim1;
Encoders encoders;

extern bool tftPushed, pushToTftInProgress;

void setup() {
    owl.setup();

    encoders.insertListener(&graphics.params);

#ifdef USE_SCREEN
    // clearPin(OLED_RST_GPIO_Port, OLED_RST_Pin);
    // HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET); // OLED off
    //HAL_GPIO_WritePin(GPIOD, OLED_RST_Pin, GPIO_PIN_SET);    
    setPin(LED_TEST_GPIO_Port, LED_TEST_Pin);
    setPin(LED_CONTROL_GPIO_Port, LED_CONTROL_Pin);
    extern SPI_HandleTypeDef OLED_SPI;
    graphics.begin(&OLED_SPI);
    tftPushed = true;
    //  clearPin(BACKLIGHT1_GPIO_Port, BACKLIGHT1_Pin);

    TIM1->CCR2 = 20;
    //    TIM1->CCR2 = tft_bl < 10 ? 10 : tft_bl;
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
#endif

    /*
      HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET); //
      OLED off extern SPI_HandleTypeDef OLED_SPI; graphics.begin(&OLED_SPI);

      __HAL_TIM_SET_COUNTER(&ENCODER_TIM1, INT16_MAX/2);
      __HAL_TIM_SET_COUNTER(&ENCODER_TIM2, INT16_MAX/2);
      HAL_TIM_Encoder_Start_IT(&ENCODER_TIM1, TIM_CHANNEL_ALL);
      HAL_TIM_Encoder_Start_IT(&ENCODER_TIM2, TIM_CHANNEL_ALL);
    */
    nextDrawTime = HAL_GetTick();
}

// int16_t* Encoders_get(){
void updateEncoders() {
    /*
      static int16_t encoder_values[2] = {INT16_MAX/2, INT16_MAX/2};
      int16_t value = __HAL_TIM_GET_COUNTER(&ENCODER_TIM1);
      int16_t delta = value - encoder_values[0];
      if(delta)
        graphics.params.encoderChanged(0, delta);
      encoder_values[0] = value;
      value = __HAL_TIM_GET_COUNTER(&ENCODER_TIM2);
      delta = value - encoder_values[1];
      if(delta)
        graphics.params.encoderChanged(1, delta);
      encoder_values[1] = value;
    */
}

// case ENC1_SW_Pin: // GPIO_PIN_14:
//   setButtonValue(BUTTON_A, !(ENC1_SW_GPIO_Port->IDR & ENC1_SW_Pin));
//   setButtonValue(PUSHBUTTON, !(ENC1_SW_GPIO_Port->IDR & ENC1_SW_Pin));
//   break;
// case ENC2_SW_Pin: // GPIO_PIN_4:
//   setButtonValue(BUTTON_B, !(ENC2_SW_GPIO_Port->IDR & ENC2_SW_Pin));
//   break;
// case TR_IN_A_Pin: // GPIO_PIN_11:
//   setButtonValue(BUTTON_C, !(TR_IN_A_GPIO_Port->IDR & TR_IN_A_Pin));
//   break;
// case TR_IN_B_Pin: // GPIO_PIN_10:
//   setButtonValue(BUTTON_D, !(TR_IN_B_GPIO_Port->IDR & TR_IN_B_Pin));
//   break;

EraseStorageTask erase_task;

static uint32_t prev_fps_update = 0;
static uint16_t num_frames = 0;

void loop(void) {
    uint32_t tick = HAL_GetTick();

#if defined USE_DCACHE
    // SCB_CleanInvalidateDCache_by_Addr((uint32_t*)graphics.params.user,
    // sizeof(graphics.params.user));
#endif

    // updateEncoders();


#ifdef USE_SCREEN
    if (!pushToTftInProgress) {
        if (tftPushed && tick >= nextDrawTime) {
            if (tick - prev_fps_update > 500) {
                if (prev_fps_update)
                    graphics.params.current_state.fps = num_frames * 1000 / (tick - prev_fps_update);
                num_frames = 0;
                prev_fps_update = tick;
            }

            tftPushed = false;
            num_frames++;

            //    graphics.screen.clear();
            graphics.params.updateState();
//            graphics.params.dirty |= 2;
//            graphics.params.dirty |= 63;
            nextDrawTime = tick + 20;
        }
        if (graphics.params.dirty && !tftDirtyBits) {
            graphics.draw();
//            graphics.display();
        }
        else if (tftDirtyBits) {
            graphics.display();
            // Poll button states - callbacks only get called when they'll be released
        }
        else {
            tftPushed = true;
        }
    }

    encoders.checkStatus(1, 1);
    encoders.checkSimpleStatus();
    encoders.processActions();    

#endif /* USE_SCREEN */

    owl.loop();
}
