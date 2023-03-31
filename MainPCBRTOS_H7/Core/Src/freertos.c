/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "defines.h"
#include "variables.h"
#include "LMS.h"
#include "IO.h"
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
extern struct rock rck[MAXROCKS];
extern struct rockBuf rckBuf[MAXROCKS];

extern RTC_TimeTypeDef gTime;
extern RTC_DateTypeDef gDate;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId IdleHandle;
osThreadId LMSHandle;
osThreadId HumidityHandle;
osThreadId IOHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void startIdle(void const * argument);
void startLMS(void const * argument);
void startHumidity(void const * argument);
void startIO(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
	*ppxIdleTaskStackBuffer = &xIdleStack[0];
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
	/* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
	/* USER CODE BEGIN Init */
	//Start UART
	HAL_UART_Receive_DMA(&huart2, (uint8_t *)&MEASdata_B, sizeof(MEASdata_B));

	//Recover parameters from flash memory
	Flash_Read_Data((uint32_t)FLASH_SPEED, (uint32_t *)&flashRead, 1);
	speed = flashRead[0];
	Flash_Read_Data((uint32_t)FLASH_OUTPUT, (uint32_t *)&flashRead, 1);
	output = flashRead[0];

	//Initialise the W5500 Ethernet controller
	W5500Init();
	HAL_Delay(2000);

	//Initialise humidity module
	if((ret = HAL_I2C_IsDeviceReady(&hi2c1, SHT31_ADDR, 1, HAL_MAX_DELAY)) != HAL_OK){
		//error handler
		for(;;);
	}

	reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
	LMS_getDevice();
	LMS_getOutputRange();
	LMS_getFrequencyResolution();

	//Initialise io-server UDP coms
	reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
	if(IO_openSocket() != 0)
		for(;;);
	HAL_Delay(500);
	IO_sendConfiguredParameters();
	IO_close(); //Empty buffer
	HAL_IWDG_Refresh(&hiwdg1);
	/* USER CODE END Init */

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
	/* definition and creation of Idle */
	osThreadDef(Idle, startIdle, osPriorityIdle, 0, 128);
	IdleHandle = osThreadCreate(osThread(Idle), NULL);

	/* definition and creation of LMS */
	osThreadDef(LMS, startLMS, osPriorityRealtime, 0, 8196);
	LMSHandle = osThreadCreate(osThread(LMS), NULL);

	/* definition and creation of Humidity */
	osThreadDef(Humidity, startHumidity, osPriorityNormal, 0, 512);
	HumidityHandle = osThreadCreate(osThread(Humidity), NULL);

	/* definition and creation of IO */
	osThreadDef(IO, startIO, osPriorityAboveNormal, 0, 8196);
	IOHandle = osThreadCreate(osThread(IO), NULL);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_startIdle */
/**
 * @brief  Function implementing the Idle thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_startIdle */
void startIdle(void const * argument)
{
	/* USER CODE BEGIN startIdle */
	/* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
	/* USER CODE END startIdle */
}

/* USER CODE BEGIN Header_startLMS */
/**
 * @brief Function implementing the LMS thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startLMS */
void startLMS(void const * argument)
{
	unsigned char connection = 0;
	/* USER CODE BEGIN startLMS */
	uint16_t LMS_dataAmount = 0;
	/* Infinite loop */
	for(;;){
		if(lock == 0){
			reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect); //Select the LMS WIZCHIP for communicating with the SICK laser
			if(LMS_getOneScan() == EXIT_SUCCESS){ //Get one scan from the LMS
				if(connection == 0){
					connection++;
					LMS_getOutputRange();
					LMS_getFrequencyResolution();
				}
				LMS_dataAmount = LMS_dataSplit(); //Split the raw data and convert into separate integers
				LMS_algorithm(LMS_dataAmount); //Apply the algorithm to differentiate the rocks and determine teh size
				reg_wizchip_cs_cbfunc(IO_select, IO_deselect); //Select the IO WIZCHIP for communicating with the IO-server
				if(output == 1 || output == 2) //Send Polar values to IO-server
					IO_sendPolarData(); //Send Polar values to IO-server
				else if(output == 3) //Send Cartesian values to VOCAM IO-server
					IO_sendCartesianData(); //Send Cartesian values to VOCAM IO-server
				bzero(LMS_calcDataX, sizeof(LMS_calcDataX));
				bzero(LMS_calcDataY, sizeof(LMS_calcDataY));
				bzero(LMS_measData, sizeof(LMS_measData));
			} else {
				connection = 0;
			}
		}
		osDelay(1);
	}
	// In case we accidentally leave loop
	osThreadTerminate(NULL);
	/* USER CODE END startLMS */
}

/* USER CODE BEGIN Header_startHumidity */
/**
 * @brief Function implementing the Humidity thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startHumidity */
void startHumidity(void const * argument)
{
	/* USER CODE BEGIN startHumidity */
	uint8_t I2C_recv[6];
	uint8_t I2C_trans[2];
	uint16_t val;

	float hum_rh;

	I2C_trans[0] = 0x2C;
	I2C_trans[1] = 0x06;
	/* Infinite loop */
	for(;;)
	{
		HAL_IWDG_Refresh(&hiwdg1);
		if((ret = HAL_I2C_Master_Transmit(&hi2c1, SHT31_ADDR, I2C_trans, 2, HAL_MAX_DELAY)) != HAL_OK){
			//error handler
			for(;;);
		}
		if((ret = HAL_I2C_Master_Receive(&hi2c1, SHT31_ADDR, I2C_recv, 6, HAL_MAX_DELAY)) != HAL_OK){
			//error handler
			for(;;);
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
	/* USER CODE END startHumidity */
}

/* USER CODE BEGIN Header_startIO */
/**
 * @brief Function implementing the IO thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startIO */
void startIO(void const * argument)
{
	/* USER CODE BEGIN startIO */
	char NMEArecv[7];

	short tempSpeed = 0;
	unsigned char tempOutput = 0;
	short speedOutputCount = 0;

	/* Infinite loop */
	for(;;)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
		vTaskDelay(10);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

		lock = 1;
		reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
		if(output == 0 || output == 2 || output == 3){
			IO_sendMeasurementData();
		}

		IO_recv();
		sscanf(IOrecv_B, "%[^,]", NMEArecv);
		if((strcmp(NMEArecv, "$PVRSCS") == 0)){
			sscanf(IOrecv_B, "%[^,],%d\r\n", NMEArecv, (int *)&tempSpeed);
			speedOutputCount++;
			speed = tempSpeed;
			if(tempSpeed != speed && speedOutputCount >= 500){
				speedOutputCount = 0;
				//Run once for timeout handler
				Flash_Write_NUM(FLASH_SPEED, speed);
				//Save the values in the Flash
				Flash_Write_NUM(FLASH_SPEED, speed);
			}
		} else if((strcmp((char *)NMEArecv, "$PVRSCO") == 0)) {
			sscanf(IOrecv_B, "%[^,],%d\r\n", NMEArecv, (int *)&tempOutput);
			if(tempOutput != output){
				output = tempOutput;
				//Run once for timeout handler
				Flash_Write_NUM(FLASH_OUTPUT, output);
				//Save the values in the Flash
				Flash_Write_NUM(FLASH_OUTPUT, output);
			}
		} else if((strcmp(NMEArecv, "$PVRSCK") == 0)){
			reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
			LMS_calibration();
			reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
			IO_close();
		} else if((strcmp(NMEArecv, "$PVRSCC") == 0)){
			reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
			LMS_configuration();
			LMS_getOutputRange();
			LMS_getFrequencyResolution();
			reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
			IO_sendConfiguredParameters();
			IO_close();
			bzero(NMEArecv, sizeof(NMEArecv));
			bzero(IOrecv_B, sizeof(IOrecv_B));
		}
		lock = 0;
		osDelay(200);
	}
	// In case we accidentally leave loop
	osThreadTerminate(NULL);
	/* USER CODE END startIO */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
