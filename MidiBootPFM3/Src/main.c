/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "device.h"
#include "errorhandlers.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

IWDG_HandleTypeDef hiwdg1;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_IWDG1_Init(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

void setup();
void loop(void);
void setMessage(const char* msg);

typedef  void (*pFunction)(void);

static int testMagic(){
  return *OWLBOOT_MAGIC_ADDRESS == OWLBOOT_MAGIC_NUMBER;
}

static int testLoop(){
  // Prevent reset cycles.
  // Check if we've been reset without zeroing the magic address,
  // which might indicate a corrupt firmware.
  return *OWLBOOT_MAGIC_ADDRESS == OWLBOOT_LOOP_NUMBER;
}

int testButton(){
#ifdef USE_BOOT1_PIN
  return !(BOOT1_GPIO_Port->IDR & BOOT1_Pin);
#else
  return 0;
#endif
}

static int testNoProgram(){  
  return ((*(__IO uint32_t*)(APPLICATION_ADDRESS + 4)) & 0xFFF00000 ) != 0x08000000;
  /* Check Vector Table: Test if valid Reset_Handler pointer is programmed after APPLICATION_ADDRESS */
}

static int testWatchdogReset(){
  return __HAL_RCC_GET_FLAG(RCC_FLAG_IWDG1RST) != RESET;
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  MX_GPIO_Init();
  MX_IWDG1_Init();
  
  if(testMagic()){
    setMessage("Bootloader starting");
  }else if(testButton()){
    setMessage("Bootloader requested");
  }else if(testLoop()){
    error(RUNTIME_ERROR, "Unexpected firmware reset");
  }else if(testWatchdogReset()){
    error(RUNTIME_ERROR, "Watchdog reset");
  }else if(testNoProgram()){
    error(RUNTIME_ERROR, "No valid firmware");
  }else{
    // jump to application code
      
      /* Disable all interrupts */
      __disable_irq();

      RCC->CIER = 0x00000000;

      /* Disable and reset SysTick */
      SysTick->CTRL = 0;
      SysTick->LOAD = 0;
      SysTick->VAL = 0;

      /* Clear Interrupt Enable Register & Interrupt Pending Register */
      for (int i = 0;i < 5; i++) {
        NVIC->ICER[i]=0xFFFFFFFF;
        NVIC->ICPR[i]=0xFFFFFFFF;
      }

    /* put marker in to prevent reset cycles */
    *OWLBOOT_MAGIC_ADDRESS = OWLBOOT_LOOP_NUMBER;

    /* Jump to user application */
    uint32_t JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
    pFunction jumpToApplication = (pFunction) JumpAddress;
    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
    jumpToApplication();
    for(;;);
  }
  /* Clear reset flags */
  __HAL_RCC_CLEAR_RESET_FLAGS();

  /* Clear magic */
  *OWLBOOT_MAGIC_ADDRESS = 0;

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_IWDG1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  // Initialise
  setup();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    loop();
#ifdef USE_IWDG
    IWDG1->KR = 0xaaaa; // reset the watchdog timer
#endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 120;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG1_Init(void)
{

  /* USER CODE BEGIN IWDG1_Init 0 */

  /* USER CODE END IWDG1_Init 0 */

  /* USER CODE BEGIN IWDG1_Init 1 */

  /* USER CODE END IWDG1_Init 1 */
  hiwdg1.Instance = IWDG1;
  hiwdg1.Init.Prescaler = IWDG_PRESCALER_128;
  hiwdg1.Init.Window = 8*32000/128;
  hiwdg1.Init.Reload = 8*32000/128;
  if (HAL_IWDG_Init(&hiwdg1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG1_Init 2 */

  /* USER CODE END IWDG1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, HC165_CLK_Pin|HC165_LOAD_Pin|SD_CS_Pin|TFT_CS_Pin
                          |TFT_DC_Pin|TFT_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED1_Pin */
  GPIO_InitStruct.Pin = LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : HC165_CLK_Pin HC165_LOAD_Pin SD_CS_Pin TFT_RESET_Pin */
  GPIO_InitStruct.Pin = HC165_CLK_Pin|HC165_LOAD_Pin|SD_CS_Pin|TFT_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : HC165_DATA_Pin */
  GPIO_InitStruct.Pin = HC165_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HC165_DATA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TFT_CS_Pin TFT_DC_Pin */
  GPIO_InitStruct.Pin = TFT_CS_Pin|TFT_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
#ifdef DEBUG
  __asm__("BKPT");
#else
  NVIC_SystemReset();
#endif
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
#ifdef DEBUG
  __asm__("BKPT");
#else
  NVIC_SystemReset();
#endif
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
