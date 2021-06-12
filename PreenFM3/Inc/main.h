/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define HC165_CLK_Pin GPIO_PIN_0
#define HC165_CLK_GPIO_Port GPIOF
#define HC165_LOAD_Pin GPIO_PIN_1
#define HC165_LOAD_GPIO_Port GPIOF
#define HC165_DATA_Pin GPIO_PIN_2
#define HC165_DATA_GPIO_Port GPIOF
#define LED_CONTROL_Pin GPIO_PIN_1
#define LED_CONTROL_GPIO_Port GPIOB
#define BACKLIGHT_PWM_Pin GPIO_PIN_11
#define BACKLIGHT_PWM_GPIO_Port GPIOE
#define SD_CS_Pin GPIO_PIN_12
#define SD_CS_GPIO_Port GPIOE
#define BACKLIGHT1_Pin GPIO_PIN_11
#define BACKLIGHT1_GPIO_Port GPIOB
#define OLED_DC_Pin GPIO_PIN_8
#define OLED_DC_GPIO_Port GPIOD
#define OLED_RST_Pin GPIO_PIN_9
#define OLED_RST_GPIO_Port GPIOD
#define OLED_CS_Pin GPIO_PIN_10
#define OLED_CS_GPIO_Port GPIOD
#define LED_TEST_Pin GPIO_PIN_1
#define LED_TEST_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
