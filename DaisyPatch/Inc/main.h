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
#define ENC_B_Pin GPIO_PIN_8
#define ENC_B_GPIO_Port GPIOB
#define OLED_DC_Pin GPIO_PIN_4
#define OLED_DC_GPIO_Port GPIOB
#define ENC_A_Pin GPIO_PIN_9
#define ENC_A_GPIO_Port GPIOB
#define USER_LED_Pin GPIO_PIN_7
#define USER_LED_GPIO_Port GPIOC
#define ADC1_Pin GPIO_PIN_0
#define ADC1_GPIO_Port GPIOC
#define GATE_IN1_Pin GPIO_PIN_1
#define GATE_IN1_GPIO_Port GPIOC
#define ADC3_Pin GPIO_PIN_4
#define ADC3_GPIO_Port GPIOC
#define GATE_IN2_Pin GPIO_PIN_6
#define GATE_IN2_GPIO_Port GPIOA
#define ENC_CLICK_Pin GPIO_PIN_12
#define ENC_CLICK_GPIO_Port GPIOB
#define ADC2_Pin GPIO_PIN_3
#define ADC2_GPIO_Port GPIOA
#define ADC4_Pin GPIO_PIN_7
#define ADC4_GPIO_Port GPIOA
#define GATE_OUT_Pin GPIO_PIN_1
#define GATE_OUT_GPIO_Port GPIOB
#define OLED_RST_Pin GPIO_PIN_15
#define OLED_RST_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
