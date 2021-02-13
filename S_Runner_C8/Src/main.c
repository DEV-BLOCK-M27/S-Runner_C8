/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#define KEY_PIN         button_in
#define KEY0            0
#define KEY1            1
#define KEY2            2
#define ALL_KEYS        (1<<KEY0 | 1<<KEY1 | 1<<KEY2)
#define REPEAT_MASK     (1<<KEY1 | 1<<KEY2)       // repeat: key1, key2
#define REPEAT_START    50                        // after 500ms
#define REPEAT_NEXT     20                        // every 200ms

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Debounce
	uint16_t get_key_press( uint16_t key_mask );
	uint16_t get_key_rpt( uint16_t key_mask );
	uint16_t get_key_state( uint16_t key_mask );
	uint16_t get_key_short( uint16_t key_mask );
	uint16_t get_key_long( uint16_t key_mask );
	volatile uint16_t key_state;                                // debounced and inverted key state:                                                  // bit = 1: key pressed
	volatile uint16_t key_press;                                // key press detect
	volatile uint16_t key_rpt;
	volatile uint16_t button_in;		// map input buttons
	volatile uint16_t buttons_pressed;  // buttons debounced


	uint8_t test;
	uint8_t	uart_flag;
uint16_t tim3_count;
uint8_t wait_for_clock_old;
uint8_t wait_for_clock;
uint8_t mode;
uint8_t mode_old;
uint8_t buttonin;
uint8_t buffer[1];
uint8_t buffersize = 4;
uint8_t clockreceived;
uint8_t clock_now;
uint8_t midi_clock = 0xf8; //
uint8_t midi_start = 0xfa;
uint8_t midi_stop  = 0b11111100;
uint8_t audio_clockdetected;
volatile uint8_t pulse_received;
volatile uint16_t pulse_count;
uint8_t midi_running;
uint8_t midi_status;
uint8_t midi_statusold;
uint8_t midi_statuschanged;
volatile uint8_t audio_gapdetected;


int main(void)
{
  HAL_Init();
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  USART1-> CR1 |= (1<<6);
//  USART1-> CR1 |= (1<<7);
  AFIO->MAPR |= (1<<24);
//  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, SET);

  NVIC_EnableIRQ(USART1_IRQn);
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_Base_Start(&htim3);

  //infiniti loop
  while (1)
  {

		//Get Buttons
		if(HAL_GPIO_ReadPin(Button_1_GPIO_Port, Button_1_Pin)){
			button_in |= (1<<0);
		}
		else{
			button_in &= ~(1<<0);
		}
		if(HAL_GPIO_ReadPin(Button_2_GPIO_Port, Button_2_Pin)){
			button_in |= (1<<1);
		}
		else{
			button_in &= ~(1<<1);
		}
		//Debounce
		buttons_pressed = get_key_press(~button_in);


		// Pushbutton 1: Wait for MIDI clock
		if(buttons_pressed &(1<<0)){
			wait_for_clock  ^= (1<<0);
//			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
		}
		// Pushbutton 2: Switch Mode - Mode 2 with 2 bars count-in
		if(buttons_pressed &(1<<1)){

			mode ^= (1<<0);
		}
		// Switch Mode LED
		  if(mode != mode_old){
			  mode_old = mode;
			  if(mode){
				  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, SET);
			  }
			  else{
				  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, RESET);
			  }
		  }

//		  // Set next Trigger for clock
	  if(wait_for_clock != wait_for_clock_old){
		  wait_for_clock_old = wait_for_clock;
		  if(wait_for_clock ){
			  USART1-> CR1 |= (1<<3);
			  buffer[0] = midi_start;
			  HAL_UART_Transmit_IT(&huart1, buffer, sizeof(buffer));
			  clock_now = 1;
			  HAL_TIM_Base_Start(&htim2);
		      TIM2->CNT = 0;
		      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, SET);
		  }
		  else{
			  USART1-> CR1 |= (1<<3);
			  buffer[0] = midi_stop;
			  HAL_UART_Transmit_IT(&huart1, buffer, sizeof(buffer));
			  clock_now = 0;
			  HAL_TIM_Base_Stop(&htim2);
			  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, RESET);
		  }
	  }

//	  if(pulse_count == 192){
//		  pulse_count = 0;
//		  clock_now = 1;
//	  }

//	  if(pulse_received){
//			pulse_received = 0;
//			midi_status = 1;
//			HAL_TIM_Base_Start(&htim2);
//			TIM2->CNT = 0;
//		}
//
//		if(audio_gapdetected){
//			audio_gapdetected = 0;
//			midi_status = 0;
//			HAL_TIM_Base_Stop(&htim2);
//		}
//
//		if(midi_status !=midi_statusold){
//			midi_statuschanged = 1;
//			midi_statusold = midi_status;
//		}
//		if(midi_statuschanged){
//			midi_statuschanged = 0;
//			if(midi_status){
//				// send MIDI start
//				USART1-> CR1 |= (1<<3);
//				buffer[0] = midi_start;
//				HAL_UART_Transmit_IT(&huart1, buffer, sizeof(buffer));
////				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
//				}
//				// send MIDI stop
//			else{
//				USART1-> CR1 |= (1<<3);
//				buffer[0] = midi_stop;
//				HAL_UART_Transmit_IT(&huart1, buffer, sizeof(buffer));
////				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
//				}
//			}
	  }//end while
	}// end main



	void TIM2_IRQHandler(void)
	{
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
//	   if(midi_status){
//		  audio_gapdetected = 1;
//	  }
//	if(wait_for_clock && midi_status == 0){
	   tim3_count++;
	   if(tim3_count == 2)
		   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

	   if(tim3_count == 15){
		   tim3_count = 0;
		   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	   }
	  HAL_TIM_IRQHandler(&htim2);

//	}
	}


	void TIM3_IRQHandler(void)
	{
		static uint16_t ct0 = 0xFF, ct1 = 0xFF,  rpt;
		uint16_t i;

		i = key_state ^ ~KEY_PIN;                       // key changed ?
		ct0 = ~( ct0 & i );                             // reset or count ct0
		ct1 = ct0 ^ (ct1 & i);                          // reset or count ct1
		i &= ct0 & ct1;                                 // count until roll over ?
		key_state ^= i;                                 // then toggle debounced state
		key_press |= key_state & i;                     // 0->1: key press detect

		if( (key_state & REPEAT_MASK) == 0 )            // check repeat function
			rpt = REPEAT_START;                          // start delay
		if( --rpt == 0 ){
			rpt = REPEAT_NEXT;                            // repeat delay
			key_rpt |= key_state & REPEAT_MASK;
		}
		HAL_TIM_IRQHandler(&htim3);

	}

	void USART1_IRQHandler(void)
	{
	  /* USER CODE BEGIN USART1_IRQn 0 */
		uart_flag++;
//		USART1-> SR &= ~(1<<6);

//		EXTI->IMR |= (1<<0);
		test = USART1-> DR;
		USART1-> CR1 &= ~(1<<3);
	  /* USER CODE END USART1_IRQn 0 */
	  HAL_UART_IRQHandler(&huart1);
	  /* USER CODE BEGIN USART1_IRQn 1 */

	  /* USER CODE END USART1_IRQn 1 */
	}


	void EXTI0_IRQHandler(void)
	{
		if(clock_now){ //&& pulse_count == 0
						USART1-> CR1 |= (1<<3);
//						USART1->DR = midi_clock;
//						test = USART1->SR (1<<6);
//						while(USART1->SR &(1<<6));+
						buffer[0] = midi_clock;
						HAL_UART_Transmit_IT(&huart1, buffer, 1);
//						HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
						uart_flag = 0;
						pulse_received = 1;
					}
					else{
						pulse_count++;
					}
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
	}
//
	void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
	{
	    if ( GPIO_Pin == GPIO_PIN_0)
	    {

	    }
	}

	// Button debouncing

	///////////////////////////////////////////////////////////////////
	//
	// check if a key has been pressed. Each pressed key is reported
	// only once
	//
	uint16_t get_key_press( uint16_t key_mask )
	{
		HAL_TIM_Base_Stop(&htim3);                                          // read and clear atomic !
		key_mask &= key_press;                          // read key(s)
		key_press ^= key_mask;                          // clear key(s)
		HAL_TIM_Base_Start(&htim3);
		return key_mask;
	}

	///////////////////////////////////////////////////////////////////
	//
	// check if a key has been pressed long enough such that the
	// key repeat functionality kicks in. After a small setup delay
	// the key is reported being pressed in subsequent calls
	// to this function. This simulates the user repeatedly
	// pressing and releasing the key.
	//
	uint16_t get_key_rpt( uint16_t key_mask )
	{
		HAL_TIM_Base_Stop(&htim3);                                          // read and clear atomic !
		key_mask &= key_rpt;                            // read key(s)
		key_rpt ^= key_mask;                            // clear key(s)
		HAL_TIM_Base_Start(&htim3);
		return key_mask;
	}

	///////////////////////////////////////////////////////////////////
	//
	// check if a key is pressed right now
	//
	uint16_t get_key_state( uint16_t key_mask )

	{
		key_mask &= key_state;
		return key_mask;
	}

	///////////////////////////////////////////////////////////////////
	//
	uint16_t get_key_short( uint16_t key_mask )
	{
		HAL_TIM_Base_Stop(&htim3);                                          // read key state and key press atomic !
		return get_key_press( ~key_state & key_mask );
	}

	///////////////////////////////////////////////////////////////////
	//
	uint16_t get_key_long( uint16_t key_mask )
	{
		return get_key_press( get_key_rpt( key_mask ));
	}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7200-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 800;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
 // htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 3600-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 90;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
	  huart1.Init.BaudRate = 31250;
	  huart1.Init.WordLength = UART_WORDLENGTH_8B;
	  huart1.Init.StopBits = UART_STOPBITS_1;
	  huart1.Init.Parity = UART_PARITY_NONE;
	  huart1.Init.Mode = UART_MODE_TX;
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
//static void MX_USART2_UART_Init(void)
//{
//
//  /* USER CODE BEGIN USART2_Init 0 */
//
//  /* USER CODE END USART2_Init 0 */
//
//  /* USER CODE BEGIN USART2_Init 1 */
//
//  /* USER CODE END USART2_Init 1 */
//  huart2.Instance = USART2;
//  huart2.Init.BaudRate = 115200;
//  huart2.Init.WordLength = UART_WORDLENGTH_8B;
//  huart2.Init.StopBits = UART_STOPBITS_1;
//  huart2.Init.Parity = UART_PARITY_NONE;
//  huart2.Init.Mode = UART_MODE_TX_RX;
//  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
//  if (HAL_UART_Init(&huart2) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN USART2_Init 2 */
//
//  /* USER CODE END USART2_Init 2 */
//
//}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();


  /*Configure GPIO pin Output Level */

  HAL_GPIO_WritePin(GPIOB, LED1_Pin|LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /*Configure GPIO pins : Trigger_Pin Button_1_Pin Button_2_Pin */
  GPIO_InitStruct.Pin = Button_1_Pin|Button_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, LED1_Pin|LED2_Pin, GPIO_PIN_RESET);



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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/