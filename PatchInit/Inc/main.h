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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SW1_Pin GPIO_PIN_8
#define SW1_GPIO_Port GPIOB
#define GATE_IN2_Pin GPIO_PIN_14
#define GATE_IN2_GPIO_Port GPIOG
#define GATE_IN2_EXTI_IRQn EXTI15_10_IRQn
#define GATE_IN1_Pin GPIO_PIN_13
#define GATE_IN1_GPIO_Port GPIOG
#define GATE_IN1_EXTI_IRQn EXTI15_10_IRQn
#define SW2_Pin GPIO_PIN_9
#define SW2_GPIO_Port GPIOB
#define SW2_EXTI_IRQn EXTI9_5_IRQn
#define GATE_OUT1_Pin GPIO_PIN_13
#define GATE_OUT1_GPIO_Port GPIOC
#define GATE_OUT2_Pin GPIO_PIN_14
#define GATE_OUT2_GPIO_Port GPIOC
#define USER_LED_Pin GPIO_PIN_7
#define USER_LED_GPIO_Port GPIOC
#define ADC5_Pin GPIO_PIN_0
#define ADC5_GPIO_Port GPIOC
#define ADC7_Pin GPIO_PIN_1
#define ADC7_GPIO_Port GPIOC
#define DAC2_Pin GPIO_PIN_4
#define DAC2_GPIO_Port GPIOA
#define ADC3_Pin GPIO_PIN_4
#define ADC3_GPIO_Port GPIOC
#define ADC6_Pin GPIO_PIN_2
#define ADC6_GPIO_Port GPIOA
#define ADC4_Pin GPIO_PIN_6
#define ADC4_GPIO_Port GPIOA
#define DAC1_Pin GPIO_PIN_5
#define DAC1_GPIO_Port GPIOA
#define ADC2_Pin GPIO_PIN_3
#define ADC2_GPIO_Port GPIOA
#define ADC8_Pin GPIO_PIN_7
#define ADC8_GPIO_Port GPIOA
#define ADC1_Pin GPIO_PIN_1
#define ADC1_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
