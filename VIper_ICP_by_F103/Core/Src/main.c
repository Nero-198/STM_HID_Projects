/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f1xx.h"
#include "usbd_customhid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/*------EPIN size over wrap------*/
/*  --この記述上手く動かないっぽい
#ifdef CUSTOM_HID_EPIN_SIZE
	#undef CUSTOM_HID_EPIN_SIZE
	#define CUSTOM_HID_EPIN_SIZE	0x11U	//0d17
#endif
#ifndef CUSTOM_HID_EPIN_SIZE
	#define CUSTOM_HID_EPIN_SIZE	0x11U	//0d17
#endif
*/
/*------END EPIN size over wrap------*/
#define __UINT16_TO_UINT8_LOW(NUMuint16) ((uint8_t)(NUMuint16))
#define __UINT16_TO_UINT8_HIGH(NUMuint16) ((uint8_t)((NUMuint16) >> 8))
#define    cbi( addr, bit)       addr &= ~(1 << bit)    // addrのbit目を'0'にする。
#define    sbi( addr, bit)       addr |= (1 << bit)       // addrのbit目を'1'にする。
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/*------Setting Device Inputs------*/
#define NUM_of_ADC_12bit 6
#define NUM_of_Buttons 40
/*------END Setting Device Inputs------*/
#define ADC_CONVERTED_DATA_BUFFER_SIZE	((uint32_t)	NUM_of_ADC_12bit)
#define BUTTONS_DATA_BUFFER_SIZE ((uint32_t)NUM_of_Buttons / 8)
/*------ KeyMatrix INPUT and OUTPUT Pin Counts ------*/
#define NUM_of_IN 4
#define NUM_of_OUT 6

typedef struct{
uint8_t buttons[BUTTONS_DATA_BUFFER_SIZE];
int8_t axis[NUM_of_ADC_12bit * 2];
}gamepadHID_t;
gamepadHID_t gamepadHID;

ADC_ChannelConfTypeDef sConfig;
uint16_t ADC_val[ADC_CONVERTED_DATA_BUFFER_SIZE];
uint16_t Buttos_Val[BUTTONS_DATA_BUFFER_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint16_t aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE];
uint8_t GPIO[2];

void keyMatrix(gamepadHID_t *gamepad){
	uint16_t buttons[NUM_of_OUT];
	GPIOB->ODR = 0x003E;		//0011,1110	//OUT0 だけLOWにする
	buttons[0] = GPIOB->IDR & 0b0000001111000000;
	buttons[0] = buttons[0] >> 6;
	GPIOB->ODR = 0x003D;		//OUT1 だけLOWにする
	buttons[1] = GPIOB->IDR & 0b0000001111000000;
	buttons[1] = buttons[1] >> 6;
	GPIOB->ODR = 0x003B;
	buttons[2] = GPIOB->IDR & 0b0000001111000000;
	buttons[2] = buttons[2] >> 6;
	GPIOB->ODR = 0x0037;
	buttons[3] = GPIOB->IDR & 0b0000001111000000;
	buttons[3] = buttons[3] >> 6;
	GPIOB->ODR = 0x002F;
	buttons[4] = GPIOB->IDR & 0b0000001111000000;
	buttons[4] = buttons[4] >> 6;
	GPIOB->ODR = 0x001F;
	buttons[5] = GPIOB->IDR & 0b0000001111000000;
	buttons[5] = buttons[5] >> 6;
	gamepad->buttons[0] = ~(uint8_t)(buttons[0] | (buttons[1] << 4));
	gamepad->buttons[1] = ~(uint8_t)(buttons[2] | (buttons[3] << 4));
	gamepad->buttons[2] = ~(uint8_t)(buttons[4] | (buttons[5] << 4));
}
void Joy_to_Buttons(gamepadHID_t *gamepad){			//要リファクタリング  なぜなら，この関数はgamepad->[3]の6bit目など 専用関数となるから．＋ sbi,cbiマクロを利用するから
	if(gamepad->axis[3] > 100){		//DCS 上
		sbi(gamepad->buttons[3]	, 6);
		cbi(gamepad->buttons[3]	, 7);
	}else if(gamepad->axis[3] < -100){		//DCS 下
		sbi(gamepad->buttons[3]	, 7);
		cbi(gamepad->buttons[3]	, 6);
	}else{		//DCS 中立
		cbi(gamepad->buttons[3]	, 6);
		cbi(gamepad->buttons[3]	, 7);
	}
	if(gamepad->axis[1] > 100){		//DCS SEQ
		sbi(gamepad->buttons[4]	, 1);
		cbi(gamepad->buttons[4]	, 0);
	}else if(gamepad->axis[1] < -100){		//DCS RTN
		sbi(gamepad->buttons[4]	, 0);
		cbi(gamepad->buttons[4]	, 1);
	}else{		//DCS 中立
		cbi(gamepad->buttons[4]	, 0);
		cbi(gamepad->buttons[4]	, 1);
	}
}
void Way3_Switch(gamepadHID_t *gamepad){
	if(!(GPIOB->IDR & (1 << 12) )){
		sbi(gamepad->buttons[3], 2);
		cbi(gamepad->buttons[3], 1);
	}
	else if(!(GPIOB->IDR & (1 << 13))){
		sbi(gamepad->buttons[3], 0);
		cbi(gamepad->buttons[3], 1);
	}
	else{
		sbi(gamepad->buttons[3], 1);
		cbi(gamepad->buttons[3], 0);
		cbi(gamepad->buttons[3], 2);
	}
	if(!(GPIOB->IDR & (1 << 14))){
		sbi(gamepad->buttons[3], 5);
		cbi(gamepad->buttons[3], 4);
	}
	else if(!(GPIOB->IDR & (1 << 15))){
		sbi(gamepad->buttons[3], 3);
		cbi(gamepad->buttons[3], 4);
	}
	else{
		sbi(gamepad->buttons[3], 4);
		cbi(gamepad->buttons[3], 3);
		cbi(gamepad->buttons[3], 5);
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */


	for (uint8_t i = 0; i < BUTTONS_DATA_BUFFER_SIZE; i++)
	{
		gamepadHID.buttons[i] = 0;
	}
	for (uint8_t i = 0; i < (NUM_of_ADC_12bit * 2); i++) {
		gamepadHID.axis[i] = 0;
	}
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  /* ### - 4 - Start conversion  mode ################################# */
  if (HAL_ADCEx_Calibration_Start(&hadc1) !=  HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  keyMatrix(&gamepadHID.buttons);
	  for(int i=0; i < NUM_of_ADC_12bit; i++){
	          HAL_ADC_Start(&hadc1);
	          HAL_ADC_PollForConversion(&hadc1, 100);
	          aADCxConvertedData[i] = HAL_ADC_GetValue(&hadc1); //12bit <<4 ~=16bit
	          aADCxConvertedData[i] = (aADCxConvertedData[i] << 4) - 0x7FF0;
	      }
	  //aADCxConvertedData[4] = aADCxConvertedData[4]*2;

	  	  //gamepadHID.buttons[0] = ~GPIO[0];
	 	  //gamepadHID.buttons[1] = ~((GPIO[1] & 0b11000000) >> 6);//gamepad buttonを左詰めにするためにLSBの方にビットシフトして???��?��??��?��?る�??

	  for(int i=0; i < NUM_of_ADC_12bit; i++){
		  gamepadHID.axis[i * 2] = __UINT16_TO_UINT8_LOW(aADCxConvertedData[i]);
		  gamepadHID.axis[(i * 2) + 1] = __UINT16_TO_UINT8_HIGH(aADCxConvertedData[i]);
	  }
	  Joy_to_Buttons(&gamepadHID);
	  Way3_Switch(&gamepadHID);

	 	//  HAL_GPIO_WritePin(OUT_D6_GPIO_Port, OUT_D6_Pin, 0);

	 	  USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, &gamepadHID, sizeof(gamepadHID_t));

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	 	  //HAL_UART_Transmit(&huart1, &gamepadHID, sizeof(struct gamepadHID_t), 100);
	 	  //HAL_UART_Transmit(&huart1, &CR, sizeof(CR), 100);
	 	  //HAL_UART_Transmit(&huart1, &aADCxConvertedData, sizeof(aADCxConvertedData[0])*5, 100);
	 	  //HAL_UART_Transmit(&huart1, &CR, sizeof(CR), 100);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = ENABLE;
  hadc1.Init.NbrOfDiscConversion = 1;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 6;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, OUT0_Pin|OUT1_Pin|OUT2_Pin|OUT3_Pin
                          |OUT4_Pin|OUT5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : TEST_LED_Pin */
  GPIO_InitStruct.Pin = TEST_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TEST_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OUT0_Pin OUT1_Pin OUT2_Pin OUT3_Pin
                           OUT4_Pin OUT5_Pin */
  GPIO_InitStruct.Pin = OUT0_Pin|OUT1_Pin|OUT2_Pin|OUT3_Pin
                          |OUT4_Pin|OUT5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : IN2_Pin IN3_Pin WARN_Pin DRIFT_Pin
                           GAIN_Pin AUTO_Pin IN0_Pin IN1_Pin */
  GPIO_InitStruct.Pin = IN2_Pin|IN3_Pin|WARN_Pin|DRIFT_Pin
                          |GAIN_Pin|AUTO_Pin|IN0_Pin|IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
