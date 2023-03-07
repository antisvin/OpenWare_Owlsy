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
#define ENC_SW_Pin GPIO_PIN_6
#define ENC_SW_GPIO_Port GPIOB
#define SW1_Pin GPIO_PIN_9
#define SW1_GPIO_Port GPIOG
#define USER_LED_Pin GPIO_PIN_7
#define USER_LED_GPIO_Port GPIOC
#define ADC_B_Pin GPIO_PIN_0
#define ADC_B_GPIO_Port GPIOC
#define LED1_R_Pin GPIO_PIN_1
#define LED1_R_GPIO_Port GPIOC
#define LED2_G_Pin GPIO_PIN_1
#define LED2_G_GPIO_Port GPIOA
#define ENC_B_Pin GPIO_PIN_0
#define ENC_B_GPIO_Port GPIOA
#define LED2_B_Pin GPIO_PIN_4
#define LED2_B_GPIO_Port GPIOA
#define ADC_A_Pin GPIO_PIN_4
#define ADC_A_GPIO_Port GPIOC
#define ENC_A_Pin GPIO_PIN_11
#define ENC_A_GPIO_Port GPIOD
#define SW2_Pin GPIO_PIN_2
#define SW2_GPIO_Port GPIOA
#define LED1_G_Pin GPIO_PIN_6
#define LED1_G_GPIO_Port GPIOA
#define LED1_B_Pin GPIO_PIN_7
#define LED1_B_GPIO_Port GPIOA
#define LED2_R_Pin GPIO_PIN_1
#define LED2_R_GPIO_Port GPIOB
#define CODEC_RESET1_Pin GPIO_PIN_11
#define CODEC_RESET1_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
