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
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "socket.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "w5500_spi.h"
#include "FLASH_SECTOR_H7.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
struct lengthPoint{
	uint8_t lengthStep;
	uint32_t ticks[500];
};

struct rock{
	uint16_t startPointX;
	uint16_t endPointX;
	struct lengthPoint lengthS;
	int width;
	int length;
	int height;
};

struct rock rck[5];
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI								3.142857
#define CORRECTION						2.5

#define PORT							5005
#define NMEA							"$PRSCD"
#define DATAPOINTMAX					512

#define TELEGRAM_LOGIN					"sMN SetAccessMode 04 81BE23AA"
#define TELEGRAM_SETSCAN_ONE			"sMN mLMPsetscancfg"
#define TELEGRAM_SETSCAN_TWO			"1"
#define TELEGRAM_SETSCAN_THREE			"FFF92230 225510"
#define TELEGRAM_OUTPUT_ONE				"sWN LMPoutputRange 1 9C4"
#define TELEGRAM_CONFIGURE_FOUR			"sWN LFPmeanfilter 0 +3 0"
#define TELEGRAM_STORE					"sMN mEEwriteall"
#define TELEGRAM_LOGOUT					"sMN Run"
#define TELEGRAM_DEVICESTATE			"sRN SCdevicestate"
#define TELEGRAM_REBOOT					"sMN mSCreboot"
#define TELEGRAM_SCAN_ONE	 			"sRN LMDscandata"

//Flash memory allocations
#define FLASH_SPEED						0x08021000
#define FLASH_PARAMETER					0x08041000

#define THRESHOLD						1340 //in millimeters
#define BELT							1410 //in millimeters
#define MAX_DIFFERENCE_X				5
#define MAX_DIFFERENCE_Y				15

#define SHT31_ADDR	0x44<<1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* Definitions for IO */
osThreadId_t IOHandle;
const osThreadAttr_t IO_attributes = {
  .name = "IO",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LMS */
osThreadId_t LMSHandle;
const osThreadAttr_t LMS_attributes = {
  .name = "LMS",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for HUM */
osThreadId_t HUMHandle;
const osThreadAttr_t HUM_attributes = {
  .name = "HUM",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */
const uint8_t LMS_IP[4] = {10, 16, 8, 100};
const uint8_t IO_IP[4] = {10, 16, 7, 198};

uint8_t LMS_recv[3000];
uint8_t LMS_buf[100];

uint8_t IO_recv[100];
uint8_t IO_buf[100];

uint8_t MEAS_data[100];

HAL_StatusTypeDef ret;
int retValIO;
int retVal;

int humAlertOne = 0; //Alert hum
int humAlertTwo = 0;
int measAmpMax;
int measFreq;
int measTemp;

unsigned int startAngle;
unsigned int endAngle;
float resolution;
unsigned int freq;
unsigned int speed;

float flashRead[4];

uint8_t lock = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI4_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void StartIO(void *argument);
void StartLMS(void *argument);
void StartHUM(void *argument);

/* USER CODE BEGIN PFP */

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
  //Read the flash for the parameters needed for the algorithm
  Flash_Read_Data(FLASH_PARAMETER, (uint32_t *)flashRead, sizeof(flashRead));
  startAngle = flashRead[0];
  endAngle = flashRead[1];
  resolution = flashRead[2];
  freq = flashRead[3];
  speed = Flash_Read_NUM(FLASH_SPEED);
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_SPI4_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  //Initialise the W5500 Ethernet controller
  W5500Init();

  //Initialise humidity module
  if((ret = HAL_I2C_IsDeviceReady(&hi2c1, SHT31_ADDR, 1, HAL_MAX_DELAY)) != HAL_OK){
	  //error handler
	  while(1);
  }

  //Initialise Ethernet module for IO-server
  reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
	  //error handler
	  while(1);
  }

  //Start DMA for UART communication with measurement PCB
  HAL_UART_Receive_DMA(&huart1, MEAS_data, 22);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of IO */
  IOHandle = osThreadNew(StartIO, NULL, &IO_attributes);

  /* creation of LMS */
  LMSHandle = osThreadNew(StartLMS, NULL, &LMS_attributes);

  /* creation of HUM */
  HUMHandle = osThreadNew(StartHUM, NULL, &HUM_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 110;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_SPI1;
  PeriphClkInitStruct.PLL3.PLL3M = 5;
  PeriphClkInitStruct.PLL3.PLL3N = 56;
  PeriphClkInitStruct.PLL3.PLL3P = 4;
  PeriphClkInitStruct.PLL3.PLL3Q = 4;
  PeriphClkInitStruct.PLL3.PLL3R = 4;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL3;
  PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C1235CLKSOURCE_PLL3;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0xC010151E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
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
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_RTS;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_ETH_A_GPIO_Port, CS_ETH_A_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, RST_ETH_A_Pin|RST_ETH_B_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_ETH_B_GPIO_Port, CS_ETH_B_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : CS_ETH_A_Pin */
  GPIO_InitStruct.Pin = CS_ETH_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_ETH_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : INT_ETH_A_Pin */
  GPIO_InitStruct.Pin = INT_ETH_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(INT_ETH_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RST_ETH_A_Pin RST_ETH_B_Pin */
  GPIO_InitStruct.Pin = RST_ETH_A_Pin|RST_ETH_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_ETH_B_Pin */
  GPIO_InitStruct.Pin = CS_ETH_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_ETH_B_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : INT_ETH_B_Pin */
  GPIO_InitStruct.Pin = INT_ETH_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(INT_ETH_B_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
  HAL_UART_Receive_DMA(&huart1, MEAS_data, 22);
  sscanf((char *)MEAS_data, "%d,%d,%d,%d", &measAmpMax, &measFreq, &measTemp, &humAlertTwo);
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartIO */
/**
  * @brief  Function implementing the IO thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartIO */
void StartIO(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
	  char NMEArecv[6];

	  int tempSpeed = 0;

	  char LMS_state = '0';
	  uint8_t LMS_dataCounter = 0;
	  float flashArr[4];

	  unsigned int resolutionConv = 0;
	  unsigned int freqConv = 0;
	  unsigned int startAngleConv = 0;
	  unsigned int endAngleConv = 0;
	  /* Infinite loop */
	  for(;;)
	  {
		  	lock = 1;
		  	reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
			sprintf((char *)IO_buf, "%s,%d,%d,%d,%d,%d,%d\r\n", NMEA, measTemp, measAmpMax, measFreq, rck[0].height, rck[0].width, rck[0].length);
			if((retValIO = sendto(1, IO_buf, strlen(IO_buf), (uint8_t *)IO_IP, PORT)) < 0){
				//error handler
			}
			if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5) == GPIO_PIN_RESET){//check interrupt pin
				if((retValIO = recvfrom(1, IO_recv, sizeof(IO_recv), (uint8_t *)IO_IP, 0)) < 0){
					//error handler
				} else{
					for(int x = retValIO; x <= 50-1; x++){
						IO_recv[x] = 0;
					}
				}
				/* Reset interrupt pin after reading message */
				setSn_IR(1, 0x04);
				setSIR(0x00);
				sscanf((char *)IO_recv, " %[^,]", NMEArecv);
				if(strcmp((char *)NMEArecv, "$PVRSCD") == 0){
					sscanf((char *)IO_recv, "%[^,],%d", NMEArecv, &tempSpeed);
					if(tempSpeed != speed){
						speed = tempSpeed;
						//Run once for timeout handler
						Flash_Write_NUM(FLASH_SPEED, speed);
						//Save the values in the Flash
						Flash_Write_NUM(FLASH_SPEED, speed);
					}
				}else if(strcmp((char *)NMEArecv, "$PVRSCC") == 0){
					sscanf((char *)IO_recv, "%[^,],%d,%d,%f,%d", NMEArecv, &startAngle, &endAngle, &resolution, &freq);
					flashArr[0] = startAngle;
					flashArr[1] = endAngle;
					flashArr[2] = resolution;
					flashArr[3] = freq;
					//Run once for timeout handler.
					Flash_Write_Data(FLASH_PARAMETER, (uint32_t *)flashArr, sizeof(flashArr));
					//Save the values in the Flash
					Flash_Write_Data(FLASH_PARAMETER, (uint32_t *)flashArr, sizeof(flashArr));
					//configure 2D LiDAR sensor with the given parameters
					if(strcmp((char *)NMEArecv, "$PVRSCC") == 0){
						reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
						if((retVal = socket(0, Sn_MR_TCP, 0, 0)) == 0){
							if((retVal = connect(0, (uint8_t *)LMS_IP, 2111)) == SOCK_OK){ //Open socket with LMS
								sprintf((char *)LMS_buf, "%c%s%c", 0x02, TELEGRAM_LOGIN, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
								if((retVal = send(0, LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
									//error handler
									for(;;);
								}
								if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the data packet from the LMS
									//error handler
									for(;;);
								}

								/* configure setscan parameter LMS */
								freqConv = freq * 100;
								resolutionConv = resolution * 10000;
								sprintf((char *)LMS_buf, "%c%s %X %s %X %s%c", 0x02, TELEGRAM_SETSCAN_ONE, freqConv, TELEGRAM_SETSCAN_TWO, resolutionConv, TELEGRAM_SETSCAN_THREE, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
								if((retVal = send(0, LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
									//error handler
									for(;;);
								}
								if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the data packet from the LMS
									//error handler
									for(;;);
								}

								/* configure output parameter LMS */
								startAngleConv = startAngle * 10000;
								endAngleConv = endAngle * 10000;
								sprintf((char *)LMS_buf, "%c%s %X %X%c", 0x02, TELEGRAM_OUTPUT_ONE, startAngleConv, endAngleConv, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
								if((retVal = send(0, LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
									//error handler
									for(;;);
								}
								if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the data packet from the LMS
									//error handler
									for(;;);
								}

								sprintf((char *)LMS_buf, "%c%s%c", 0x02, TELEGRAM_CONFIGURE_FOUR, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
								if((retVal = send(0, LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
									//error handler
									for(;;);
								}
								if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the data packet from the LMS
									//error handler
									for(;;);
								}

								sprintf((char *)LMS_buf, "%c%s%c", 0x02, TELEGRAM_STORE, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
								if((retVal = send(0, LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
									//error handler
									for(;;);
								}
								if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the data packet from the LMS
									//error handler
									for(;;);
								}

								sprintf((char *)LMS_buf, "%c%s%c", 0x02, TELEGRAM_LOGOUT, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
								if((retVal = send(0, LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
									//error handler
									for(;;);
								}
								if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the data packet from the LMS
									//error handler
									for(;;);
								}

								sprintf((char *)LMS_buf, "%c%s%c", 0x02, TELEGRAM_DEVICESTATE, 0x03);
								while(LMS_state != '1'){
									if((retVal = send(0, LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
										//error handler
										for(;;);
									}
									if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the data packet from the LMS
										//error handler
										for(;;);
									}
									for(int j = 0; j <= retVal-1; j++){
										if(LMS_recv[j] == 0x20){
											LMS_dataCounter++;
										}
										if(LMS_dataCounter == 2){
											j++;
											LMS_state = LMS_recv[j];
											LMS_dataCounter = 0;
											break;
										}
									}
								}
								LMS_state = '0';
								disconnect(0);
								close(0);
							}
						}
					}
					reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
					close(1);
					if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
					  //error handler
					  while(1);
					}
				}
			}
			lock = 0;
			osDelay(1000);
	  }
	  // In case we accidentally leave loop
	  osThreadTerminate(NULL);
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartLMS */
/**
* @brief Function implementing the LMS thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLMS */
void StartLMS(void *argument)
{
  /* USER CODE BEGIN StartLMS */
	  uint16_t LMS_measData[DATAPOINTMAX];
	  int16_t LMS_calcDataX[DATAPOINTMAX];
	  int16_t LMS_calcDataY[DATAPOINTMAX];
	  uint8_t LMS_dataRawAmount[4] = {0, 0, 0, 0};
	  uint16_t LMS_dataAmount = 0;
	  uint8_t LMS_dataRaw[4] = {0, 0, 0, 0};
	  uint16_t LMS_data = 0;
	  uint8_t LMS_commaCounter = 0;
	  uint16_t LMS_measArray[DATAPOINTMAX];
	  uint16_t i = 0;

	  uint8_t status = 0;
	  uint8_t statusTwo = 0;
	  uint8_t statusThree = 0;
	  uint8_t statusThreeCounter = 0;
	  uint8_t rockCount = 0;
	  int16_t LMS_dataArrayX[DATAPOINTMAX];
	  int16_t LMS_dataArrayY[DATAPOINTMAX];
	  int16_t startWidth = 0;
	  int16_t endWidth = 0;
	  uint16_t heightTemp = 0;
	  uint16_t widthTemp = 0;
	  uint8_t heightCounter = 0;

	  float angle;

	  uint8_t xAxisControl = 0;
	  uint8_t rckControl = 0;

	  uint32_t start = 0;
	  uint32_t stop = 0;
	  uint32_t delta = 0;
	  /* Infinite loop */
	  for(;;)
	  {
		if(lock == 0){
			//Timer for distance calculation
			stop  = HAL_GetTick();
			delta = stop - start;
			start = HAL_GetTick();
			reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
			if((retVal = socket(0, Sn_MR_TCP, 0, 0)) == 0){
				if((retVal = connect(0, (uint8_t *)LMS_IP, 2111)) == SOCK_OK){ //Open socket with LMS
					  sprintf((char *)LMS_buf, "%c%s%c", 0x02, TELEGRAM_SCAN_ONE, 0x03); //Create telegram to be sent to LMS (Ask for one)
					  reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
					  if((retVal = send(0, (uint8_t *)LMS_buf, strlen(LMS_buf))) <= 0){ //Send the Telegram to LMS
						  //error handler
						  for(;;);
					  }
					  if((retVal = recv(0, LMS_recv, sizeof(LMS_recv))) <= 0){ //Receive the telegram message from the LMS
						  //error handler
						  for(;;);
					  }
					  disconnect(0);
					  close(0);
					  for(int j = 0; j <= retVal-1; j++){
						  if(LMS_recv[j] == 0x20){
							  //Each data point counted after a comma
							  LMS_commaCounter++;
						  }
						  if(LMS_commaCounter == 25){
							  //on the 25th data point, the data amount can be found
							  j++;
							  while(LMS_recv[j] != 0x20){
								  LMS_dataRawAmount[i++] = LMS_recv[j++];
							  }
							  j--;
							  i = 0;
							  LMS_dataAmount = strtol((char *)LMS_dataRawAmount, NULL, 16);
						  } else if(LMS_commaCounter == 26){ //convert all the data point to single integers in an array
							  for(int x = 0; x <= LMS_dataAmount-1; x++){
								  j++;
								  while(LMS_recv[j] != 0x20){
									  LMS_dataRaw[i++] = LMS_recv[j++];
								  }
								  LMS_commaCounter++;
								  i = 0;
								  LMS_data = strtol((char *)LMS_dataRaw, NULL, 16);
								  for(int z = 0; z <= 3; z++){
									  LMS_dataRaw[z] = 0;
								  }
								  LMS_measData[x] = LMS_data;
							  }
							  j--;
						  } else if(LMS_recv[j] == 0x03){
							  LMS_commaCounter = 0;
							  //Start the algorithm
							  /* Filter */

							  /* Polar to Cartesian */
							  angle = startAngle+CORRECTION;
							  for(int x = 0; x <= LMS_dataAmount-1; x++){
								  //From Polar coordinates to cartesian coordinates.
								  LMS_calcDataX[x] = LMS_measData[x]*cos((angle*PI)/180);
								  LMS_calcDataY[x] = LMS_measData[x]*sin((angle*PI)/180);
								  angle = angle + resolution;
								  if(LMS_calcDataY[x] < THRESHOLD){
									  //Only take data below the given threshold.
									  LMS_dataArrayX[x] = LMS_calcDataX[x];
									  LMS_dataArrayY[x] = LMS_calcDataY[x];
									  LMS_measArray[x] = LMS_measData[x];
								  } else {
									  //everything else zero.
									  LMS_dataArrayX[x] = 0;
									  LMS_dataArrayY[x] = 0;
									  LMS_measArray[x] = 0;
								  }
							  }
							  /* Determine rocks */
							  for(int x = 0; x <= LMS_dataAmount-1; x++){
								  /* check rock points (X axis only)*/
								  if(LMS_measArray[x] != 0 && status == 0){
									  startWidth = LMS_dataArrayX[x];
									  if(rck[rockCount].startPointX > 0){//check if previous measurement already detected rock and calculate the length which will gradually add up
										  //check if this rck struct isn't filled with another parameter from another rock.

										  //check if the axis don't differentiate too much
										  if(rck[rockCount].startPointX <= x+MAX_DIFFERENCE_X && rck[rockCount].startPointX >= x-MAX_DIFFERENCE_X){
											  rck[rockCount].startPointX = x;
											  rck[rockCount].lengthS.lengthStep++;
											  rck[rockCount].lengthS.ticks[rck[rockCount].lengthS.lengthStep] = delta;
											  xAxisControl = 0;
										  } else { //If so than save it and wait till the algorithm checks for the next end, if it also differentiate too much
											  if(xAxisControl == 2){//if both end differentiate too much then reset PointX values and calculate the final length and height
												  rck[rockCount].startPointX = 0;
												  rck[rockCount].endPointX = 0;
											  } else {//if only one end differentiate too much, it is consider still as one rock. the measurement continues
												  xAxisControl = 1;
												  rck[rockCount].startPointX = x;
											  }
										  }
									  } else {//If not then start new rock measurement
										  rck[rockCount].startPointX = x;
										  do{
											  //compare the value with the other values to check if there is an shift in rocks.
											  if(rockCount != rckControl){
												  if(rck[rckControl].startPointX <= rck[rockCount].startPointX+MAX_DIFFERENCE_X && rck[rckControl].startPointX >= rck[rockCount].startPointX+MAX_DIFFERENCE_X){
													  rck[rckControl].startPointX = x;
													  rck[rckControl].lengthS.lengthStep++;
													  rck[rckControl].lengthS.ticks[rck[rockCount].lengthS.lengthStep] = delta;
													  break;
												  }
											  }
											  rckControl++;
										  }while(rckControl <= 4);
										  if(rckControl >= 5){
											  rck[rockCount].startPointX = x;
											  rck[rockCount].lengthS.lengthStep++;
											  rck[rockCount].lengthS.ticks[rck[rockCount].lengthS.lengthStep] = delta;
										  }
										  rckControl = 0;
									  }
									  status++;
								  } else if(LMS_measArray[x] == 0 && status != 0){
									  x--;
									  endWidth = LMS_dataArrayX[x];
									  if(rck[rockCount].endPointX > 0){//check if previous measurement already detected rock
										  //check if this erck block isn't filled with another parameter from another rock.
										  if(rck[rockCount].endPointX <= x+MAX_DIFFERENCE_X && rck[rockCount].endPointX >= x-MAX_DIFFERENCE_X){//check if the axis don't differentiate too much
											  rck[rockCount].endPointX = x;
											  xAxisControl = 0;
										  }else{//If so than save it and wait till the algorithm for the next end also differentiate too much
											  if(xAxisControl == 1){//if both end differentiate too much then reset PointX values and calculate the final length
												  rck[rockCount].startPointX = 0;
												  rck[rockCount].endPointX = 0;

											  } else {//if only one end differentiate too much, it is consider still as one rock. the measurement continues
												  xAxisControl = 2;
												  rck[rockCount].endPointX = x;
											  }
										  }
									  } else {//If not then start new rock measurement
										  rck[rockCount].endPointX = x;
										  do{
											  //compare the value with the other values to check if there is an shift in rocks.
											  if(rockCount != rckControl){
												  if(rck[rckControl].endPointX <= rck[rockCount].endPointX+MAX_DIFFERENCE_X && rck[rckControl].endPointX >= rck[rockCount].endPointX+MAX_DIFFERENCE_X){
													  rck[rckControl].endPointX = x;
													  break;
												  }
											  }
											  rckControl++;
										  }while(rckControl <= 4);
										  if(rckControl >= 5){
											  rck[rockCount].endPointX = x;
										  }
										  rckControl = 0;
									  }
									  x++;
									  rockCount++;
									  status--;
									  statusTwo = 1;
								  }

								  /* calculate dimensions rock */
								  if(rockCount >= 1 && statusTwo == 1){
									  //A status variable to check if the stone is present or not
									  statusThree = 1;
									  statusThreeCounter = 0;
									  //width
									  widthTemp = startWidth - endWidth;
									  if(widthTemp > rck[rockCount-1].width){
										  rck[rockCount-1].width = widthTemp;
									  }
									  //height
									  for(int y = rck[rockCount-1].startPointX; y <= rck[rockCount-1].endPointX; y++){
										  heightTemp += LMS_dataArrayY[y];
										  heightCounter++;
									  }
									  heightTemp = BELT - (heightTemp/heightCounter);
									  if(heightTemp <= BELT){
										  if(heightTemp > rck[rockCount-1].height){
											  rck[rockCount-1].height = heightTemp;
										  }
									  } else {
										  rck[rockCount-1].height = 0;
									  }
									  heightTemp = 0;
									  heightCounter = 0;
									  //length
									  rck[rockCount-1].length += (speed*delta)/1000;
									  statusTwo = 0;
								  }
							  }
							  if(statusThree == 0){
								  statusThreeCounter++;
								  //if after 5 data cycles no rocks are detected, reset rock values
								  if(statusThreeCounter >= 5){
									  for(int i = 0; i < 5; i++){
										  rck[i].width = 0;
										  rck[i].height = 0;
										  rck[i].length = 0;
									  }
								  }
							  }
							  statusThree = 0;
							  rockCount = 0;
							  angle = 0;
							  /* Log all data points for matlab */
	//						  for(int x = 0; x <= LMS_dataAmount-1; x++){
	//							  if(x < LMS_dataAmount-1){
	//								  sprintf((char *)UART_buf, "%d,", LMS_calcRawData[x]);
	//								  HAL_UART_Transmit(&huart2, UART_buf, strlen(UART_buf), 1);
	//							  } else if(x >= LMS_dataAmount-1) {
	//								  sprintf((char *)UART_buf, "%d,,%d\r\n", LMS_calcRawData[x], (int)delta);
	//								  HAL_UART_Transmit(&huart2, UART_buf, strlen(UART_buf), 1);
	//							  }
	//						  }
						  }
					  }
				  }
			}
		}
	    osDelay(1);
	  }
	  // In case we accidentally leave loop
	  osThreadTerminate(NULL);
  /* USER CODE END StartLMS */
}

/* USER CODE BEGIN Header_StartHUM */
/**
* @brief Function implementing the HUM thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartHUM */
void StartHUM(void *argument)
{
  /* USER CODE BEGIN StartHUM */
	uint8_t I2C_recv[6];
	uint8_t I2C_trans[2];
	uint16_t val;

	float hum_rh;

	I2C_trans[0] = 0x2C;
	I2C_trans[1] = 0x06;
  /* Infinite loop */
  for(;;)
  {
	if((ret = HAL_I2C_Master_Transmit(&hi2c1, SHT31_ADDR, I2C_trans, 2, HAL_MAX_DELAY)) != HAL_OK){
		//error handler
	}
	if((ret = HAL_I2C_Master_Receive(&hi2c1, SHT31_ADDR, I2C_recv, 6, HAL_MAX_DELAY)) != HAL_OK){
		//error handler
	}
	val = I2C_recv[3] << 8 | I2C_recv[4];
	hum_rh = 100*(val/(pow(2,16)-1));
	if(hum_rh > 75){
		humAlertOne = 1;
	}
    osDelay(500);
  }
  // In case we accidentally leave loop
  osThreadTerminate(NULL);
  /* USER CODE END StartHUM */
}

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
