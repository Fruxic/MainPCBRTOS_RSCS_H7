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
//struct lengthPoint{
//	unsigned short lengthStep;
//	unsigned int ticks[500];
//};
//
//struct rock{
//	unsigned short startIndex;
//	unsigned short startIndexMax;
//	unsigned short endIndex;
//	unsigned short endIndexMax;
//	struct lengthPoint lengthS;
//	unsigned int width;
//	unsigned int length;
//	unsigned int height;
//};
//
//struct rockBuf{
//	unsigned int width;
//	unsigned int length;
//	unsigned int height;
//};

extern struct rock rck[MAXROCKS];
extern struct rockBuf rckBuf[MAXROCKS];

RTC_TimeTypeDef gTime;
RTC_DateTypeDef gDate;
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
//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
//LMS functions
//char LMS_connect(void);
//void LMS_disconnect(void);
//void LMS_receivePoll(void);
//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
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
	//Start UART DMA
	HAL_UART_Receive_DMA(&huart2, (uint8_t *)&MEASdata_B, sizeof(MEASdata_B));

	//Recover parameters from flash memory
	Flash_Read_Data((uint32_t)FLASH_SPEED, (uint32_t *)&flashRead, 1);
	speed = flashRead[0];
	Flash_Read_Data((uint32_t)FLASH_OUTPUT, (uint32_t *)&flashRead, 1);
	output = flashRead[0];

	//Initialise the W5500 Ethernet controller
	W5500Init();
	HAL_Delay(1000);

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
	if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
		//error handler
		for(;;);
	}
	HAL_Delay(500);
	//Send what is saved in the Flash memory to the VOCAM/IO-server
	sprintf(IOtrans_B, "%s,%d,%d,%f,%d,%d,%d\r\n", NMEA_FLASH, startAngle, endAngle, resolution, freq, output, 1);
	if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
		//error handler
		close(1);
		if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
			//error handler
			NVIC_SystemReset();
			for(;;);
		}
	}
	HAL_Delay(500);
	sprintf(IOtrans_B, "%s,%d,%d,%f,%d,%d,%d\r\n", NMEA_FLASH, startAngle, endAngle, resolution, freq, output, 0);
	if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
		//error handler
		close(1);
		if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
			//error handler
			NVIC_SystemReset();
			for(;;);
		}
	}
	close(1);
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
  /* USER CODE BEGIN startLMS */
	char LMS_dataRawAmount[6] = {0, 0, 0, 0 ,0 ,0};
	uint16_t LMS_dataAmount = 0;
	char LMS_dataRaw[8] = {0, 0, 0, 0, 0 ,0 ,0 ,0};
	uint16_t LMS_data = 0;
	uint16_t LMS_commaCounter = 0;
	uint16_t LMS_measArray[DATAPOINTMAX];
	uint16_t i = 0;
	char strAppend[8];
	char strAppendX[8];
	char strAppendY[8];
	uint32_t strCount = 0;
	uint32_t strCountX = 0;
	uint32_t strCountY = 0;

	uint8_t status = 0;
	uint8_t statusTwo = 0;
	uint8_t statusThree = 0;
	uint8_t statusThreeCounter = 0;
	uint16_t indexCounter = 0;
	uint8_t rockCount = 0;
	int16_t LMS_dataArrayX[DATAPOINTMAX];
	uint16_t LMS_dataArrayY[DATAPOINTMAX];
	int32_t startWidth = 0;
	int32_t endWidth = 0;
	uint32_t heightTemp = 0;
	uint32_t widthTemp = 0;

	float angle;

	uint8_t xAxisControl = 0;
	uint8_t xAxisStatus = 0;
	uint8_t rckControl = 0;
	uint8_t xAxisRightShiftStatus = 0;
	uint8_t xAxisLeftShiftStatus = 0;
	uint16_t startIndexTemp = 0;
	uint16_t endIndexTemp = 0;
	uint8_t rckStatus = 0;

	uint32_t start = 0;
	uint32_t stop = 0;
	uint32_t delta = 0;
	/* Infinite loop */
	for(;;){
		if(lock == 0){
			//Timer for distance calculation
			stop  = HAL_GetTick();
			delta = stop - start; //Measure time interval
			start = HAL_GetTick();
			//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
			//		  reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
			//		  if(LMS_connect() == EXIT_SUCCESS){
			//			  LMS_receivePoll();
			//			  LMS_disconnect();
			//		  }
			//-------------------^STILL UNDER DEVELOPMENT^-----------------------------------//
			//---------------------------------------------Get data from LMS-----------------------------------------------------------------//
			reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
			if(((retVal = socket(0, Sn_MR_TCP, 0, 0)) == 0)){
				if((retVal = connect(0, (uint8_t *)LMS_IP, 2111)) == SOCK_OK){ //Open socket with LMS
					sprintf((char *)LMStrans_B, "%c%s%c", 0x02, TELEGRAM_SCAN_ONE, 0x03); //Create telegram to be sent to LMS (Ask for one)
					if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
						//error handler
						NVIC_SystemReset();
						for(;;);
					}
					if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the telegram message from the LMS
						//error handler
						NVIC_SystemReset();
						for(;;);
					}
					disconnect(0);
					close(0);
					//---------------------------------------------Split Data-----------------------------------------------------------------//
					for(int j = 0; j < retVal; j++){
						if(LMSrecv_B[j] == 0x20){
							//Each data point counted after a comma
							LMS_commaCounter++;
						}
						if(LMS_commaCounter == 25){
							//on the 25th data point, the data amount can be found
							j++;
							while(LMSrecv_B[j] != 0x20){
								LMS_dataRawAmount[i++] = LMSrecv_B[j++];
							}
							j--;
							i = 0;
							LMS_dataAmount = strtol((char *)LMS_dataRawAmount, NULL, 16);
							bzero(LMS_dataRawAmount, sizeof(LMS_dataRawAmount));
						} else if(LMS_commaCounter == 26){ //convert all the data point to single integers in an array
							for(int x = 0; x < LMS_dataAmount; x++){
								j++;
								while(LMSrecv_B[j] != 0x20 && LMSrecv_B[j] != 0x0){
									LMS_dataRaw[i++] = LMSrecv_B[j++];
								}
								LMS_commaCounter++;
								i = 0;
								LMS_data = strtol((char *)LMS_dataRaw, NULL, 16);
								bzero(LMS_dataRaw, sizeof(LMS_dataRaw));
								LMS_measData[x] = LMS_data;
							}
							j--;
						} else if(LMSrecv_B[j] == 0x03){
							LMS_commaCounter = 0;
							break;
						}
					}
					//-------------------------------------------------Polar to Cartesian--------------------------------------------------------------//
					angle = startAngle;
					for(int x = 0; x < LMS_dataAmount; x++){
						//From Polar coordinates to Cartesian coordinates.
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
					//-------------------------------------------------Send calculated/decoded values to IO-server--------------------------------------------------------------//
					reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
					if(output == 1 || output == 2){ //Send Polar values to IO-server
						for(int x = 0; x <= VOCAM_MAX; x++){
							if(x < VOCAM_MAX){
								sprintf((char *)strAppend, "%d,", LMS_measData[x]);
								strCount += strlen(strAppend);
							} else if(x >= VOCAM_MAX){
								sprintf((char *)strAppend, "%d\r\n", LMS_measData[x]);
								strCountX += strlen(strAppend);
							}
							if(strCount < MTU){
								strncat(LMSpointCloudPolarOne_B, strAppend, strlen(strAppend));
							} else if (strCount >= MTU && strCount < MTU*2){
								strncat(LMSpointCloudPolarTwo_B, strAppend, strlen(strAppend));
							} else if (strCount >= MTU*2 && strCount < MTU*3){
								strncat(LMSpointCloudPolarThree_B, strAppend, strlen(strAppend));
							}
						}
						sprintf(IOtrans_B, "%s,%s", NMEA_POINT, LMSpointCloudPolarOne_B);
						if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
							//error handler
							close(1);
							if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
								//error handler
								NVIC_SystemReset();
								while(1);
							}
						}
						//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
						//					  if(strCount >= MTU){
						//						  sprintf(IO_buf, "%s", LMS_pointCloudPolarTwo);
						//						  if((retValIO = sendto(1, (uint8_t *)IO_buf, strlen(IO_buf), (uint8_t *)IO_IP, PORT)) < 0){
						//							  //error handler
						//							  close(1);
						//							  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
						//								  //error handler
						//								  NVIC_SystemReset();
						//								  while(1);
						//							  }
						//						  }
						//						  if(strCount >= MTU*2){
						//							  sprintf(IO_buf, "%s", LMS_pointCloudPolarThree);
						//							  if((retValIO = sendto(1, (uint8_t *)IO_buf, strlen(IO_buf), (uint8_t *)IO_IP, PORT)) < 0){
						//								  //error handler
						//								  close(1);
						//								  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
						//									  //error handler
						//									  NVIC_SystemReset();
						//									  while(1);
						//								  }
						//							  }
						//						  }
						//					  }
						//-------------------^STILL UNDER DEVELOPMENT^-----------------------------------//
						bzero(LMSpointCloudPolarOne_B, sizeof(LMSpointCloudPolarOne_B));
						bzero(LMSpointCloudPolarTwo_B, sizeof(LMSpointCloudPolarTwo_B));
						bzero(LMSpointCloudPolarThree_B, sizeof(LMSpointCloudPolarThree_B));
						strCount = 0;
					} else if(output == 3){ //Send Cartesian values to VOCAM IO-server
						for(int x = 0; x < VOCAM_MAX; x++){
							if(x < VOCAM_MAX-1){
								sprintf((char *)strAppendX, "%d,", LMS_calcDataX[x]);
								strCountX += strlen(strAppendX);
								sprintf((char *)strAppendY, "%d,", LMS_calcDataY[x]);
								strCountY += strlen(strAppendY);
							} else if (x >= VOCAM_MAX-1) {
								sprintf((char *)strAppendX, "%d\r\n", LMS_calcDataX[x]);
								strCountX += strlen(strAppendX);
								sprintf((char *)strAppendY, "%d\r\n", LMS_calcDataY[x]);
								strCountY += strlen(strAppendY);
							}
							if(strCountX < MTU){
								strncat(LMSpointCloudXOne_B, strAppendX, strlen(strAppendX));
							} else if (strCountX >= MTU && strCountX < MTU*2){
								strncat(LMSpointCloudXTwo_B, strAppendX, strlen(strAppendX));
							} else if (strCountX >= MTU*2 && strCountX < MTU*3){
								strncat(LMSpointCloudXThree_B, strAppendX, strlen(strAppendX));
							}
							if(strCountY < MTU){
								strncat(LMSpointCloudYOne_B, strAppendY, strlen(strAppendY));
							} else if (strCountY >= MTU && strCountY < MTU*2){
								strncat(LMSpointCloudYTwo_B, strAppendY, strlen(strAppendY));
							} else if (strCountY >= MTU*2 && strCountY < MTU*3){
								strncat(LMSpointCloudYThree_B, strAppendY, strlen(strAppendY));
							}
						}
						sprintf((char *)IOtrans_B, "%s,%s", NMEA_POINTY, LMSpointCloudYOne_B);
						if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
							//error handler
							close(1);
							if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
								//error handler
								NVIC_SystemReset();
								for(;;);
							}
						}
						//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
						//					  if(strCountY >= MTU){
						//						  sprintf((char *)IO_buf, "%s", LMS_pointCloudYTwo);
						//						  if((retValIO = sendto(1, (uint8_t *)IO_buf, strlen(IO_buf), (uint8_t *)IO_IP, PORT)) < 0){
						//							  //error handler
						//							  close(1);
						//							  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
						//								  //error handler
						//								  NVIC_SystemReset();
						//								  for(;;);
						//							  }
						//						  }
						//						  if(strCountY >= MTU*2){
						//							  sprintf((char *)IO_buf, "%s", LMS_pointCloudYThree);
						//							  if((retValIO = sendto(1, (uint8_t *)IO_buf, strlen(IO_buf), (uint8_t *)IO_IP, PORT)) < 0){
						//								  //error handler
						//								  close(1);
						//								  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
						//									  //error handler
						//									  NVIC_SystemReset();
						//									  for(;;);
						//								  }
						//							  }
						//						  }
						//					  }
						//-------------------^STILL UNDER DEVELOPMENT^-----------------------------------//
						sprintf(IOtrans_B, "%s,%s", NMEA_POINTX, LMSpointCloudXOne_B);
						if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
							//error handler
							close(1);
							if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
								//error handler
								NVIC_SystemReset();
								for(;;);
							}
						}
						//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
						//					  if(strCountX >= MTU){
						//						  sprintf(IO_buf, "%s", LMS_pointCloudXTwo);
						//						  if((retValIO = sendto(1, (uint8_t *)IO_buf, strlen(IO_buf), (uint8_t *)IO_IP, PORT)) < 0){
						//							  //error handler
						//							  close(1);
						//							  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
						//								  //error handler
						//								  NVIC_SystemReset();
						//								  for(;;);
						//							  }
						//						  }
						//						  if(strCountX >= MTU*2){
						//							  sprintf(IO_buf, "%s", LMS_pointCloudXThree);
						//							  if((retValIO = sendto(1, (uint8_t *)IO_buf, strlen(IO_buf), (uint8_t *)IO_IP, PORT)) < 0){
						//								  //error handler
						//								  close(1);
						//								  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
						//									  //error handler
						//									  NVIC_SystemReset();
						//									  for(;;);
						//								  }
						//							  }
						//						  }
						//					  }
						//-------------------^STILL UNDER DEVELOPMENT^-----------------------------------//
						bzero(LMSpointCloudXOne_B, sizeof(LMSpointCloudXOne_B));
						bzero(LMSpointCloudYOne_B, sizeof(LMSpointCloudYOne_B));
						bzero(LMSpointCloudXTwo_B, sizeof(LMSpointCloudXTwo_B));
						bzero(LMSpointCloudYTwo_B, sizeof(LMSpointCloudYTwo_B));
						bzero(LMSpointCloudXThree_B, sizeof(LMSpointCloudXThree_B));
						bzero(LMSpointCloudYThree_B, sizeof(LMSpointCloudYThree_B));
						strCountX = 0;
						strCountY = 0;
					}
					bzero(LMS_calcDataX, sizeof(LMS_calcDataX));
					bzero(LMS_calcDataY, sizeof(LMS_calcDataY));
					bzero(LMS_measData, sizeof(LMS_measData));
					//-----------------------------------------------------Determine rocks----------------------------------------------------------//
					//-------------------STILL UNDER DEVELOPMENT------------------------------//
					for(int x = 0; x < LMS_dataAmount; x++){
						if((LMS_measArray[x] != 0 || rck[rockCount].startIndex == x) && status == 0){ //Check if object is present that has a Y coordinate value above the given THRESHOLD.
							startWidth = LMS_dataArrayX[x]; //Save the start width from the Cartesian X coordinates, this will be used to calculate the current width.
							if(rck[rockCount].startIndex > 0){ //Check if previous measurement already detected rock.
								startIndexTemp = rck[rockCount].startIndex; //Save previous startIndex.
								rck[rockCount].startIndex = x;  //Save current start index in the struct
								if(startIndexTemp <= x+MAXINDEXDIFF_X && startIndexTemp >= x-MAXINDEXDIFF_X && LMS_measArray[x] > 0){//check if the start index don't differentiate too much from the previous detection.
									//Keep this empty for possible future improvement
									status++;
									rckStatus = 0;
								} else if (LMS_measArray[x] <= 0){
									rckStatus++; //increment status variable for end of rock
								} else { //if both are not true, then compare the current values with the other previous values to check if there is an shift in rocks struct allocation.
									do {
										if(rockCount != rckControl){
											if(rck[rckControl].startIndex <= x+MAXINDEXDIFF_X && rck[rckControl].startIndex >= x+MAXINDEXDIFF_X){
												if(startIndexTemp < x){
													xAxisLeftShiftStatus++; //Increment status variable for left shifting of struct array if end index differs as well
												}
												break;
											}
										}
										rckControl++; //increment control variable for similarity check of each array
									} while(rckControl < MAXROCKS);
									if(rckControl >= MAXROCKS){ //if no correlation can be found with the current index value, then new rock is found before the previous detected rock
										if(startIndexTemp > x){
											xAxisRightShiftStatus++; //Increment status variable for right shifting of struct array if end index differs as well
										}
									}
									rckControl = 0;//Reset control variable for similarity check of each array
									status++;
								}
							} else { //If not then start new rock measurement
								rck[rockCount].startIndex = x;
								status++;
							}
						} else if(LMS_measArray[x] <= 0 && status != 0){ //Check if the end of object is detected or if the previous detection is
							if(indexCounter >= 3){ //If less than three indexes are filled in a row then do not consider it as an object/rock
								x--; //Decrement X value, this is because the if statement above reacts when the polar coordinate is 0
								endWidth = LMS_dataArrayX[x]; //Save the end width from the Cartesian X coordinates, this will be used to calculate the width.
								if(rck[rockCount].endIndex > 0){ //check if previous measurement already detected a rock
									endIndexTemp = rck[rockCount].endIndex; //Save previous end index.
									rck[rockCount].endIndex = x;  //Save current end index in the struct
									if(endIndexTemp <= x+MAXINDEXDIFF_X && endIndexTemp >= x-MAXINDEXDIFF_X){//Check if the end index don't differentiate too much from the previous detection.
										startIndexTemp = 0; //Reset the temporary index values if object still is present on the same location
										endIndexTemp = 0;   //Reset the temporary index values if object still is present on the same location
										rck[rockCount].lengthS.lengthStep++; //Also increment the step for calculating length
									} else { //if not then compare the current values with the other previous values to check if there is an shift in rocks struct allocation.
										do {
											if(rockCount != rckControl){
												if(rck[rckControl].endIndex <= x+MAXINDEXDIFF_X && rck[rckControl].endIndex >= x+MAXINDEXDIFF_X){
													if(endIndexTemp < x && xAxisLeftShiftStatus >= 1){ //If rock was found in a different array to the right.
														xAxisLeftShiftStatus++; //Increment status variable for left shifting of struct array.
														//for loop needed to shift everything
														rck[rockCount].lengthS.lengthStep = rck[rckControl].lengthS.lengthStep++;
														rck[rockCount].width = rck[rckControl].width;
														rck[rockCount].length = rck[rckControl].length;
														rck[rockCount].height = rck[rckControl].height;
													}
													break;
												}
											}
											rckControl++;
										} while (rckControl < MAXROCKS);
										if (rckControl >= MAXROCKS){
											if (endIndexTemp > x && xAxisRightShiftStatus >= 1){ //If rock was found in a different array to the left.
												xAxisRightShiftStatus++; //Increment status variable for right shifting of struct array
												//for loop needed to shift everything
												rck[rockCount+1].lengthS.lengthStep = rck[rockCount].lengthS.lengthStep++;
												rck[rockCount+1].width = rck[rockCount].width;
												rck[rockCount+1].length = rck[rockCount].length;
												rck[rockCount+1].height = rck[rockCount].height;
											}
										}
									}
								} else {//If not then start new rock measurement
									rck[rockCount].endIndex = x;
								}
								x++;    //Set the x variable of the for loop back to its original state
								rckControl = 0;
								rockCount++; //increment the rock count of the measurement line
								status--; //Reset status variable for start new rock detection
								statusTwo = 1; //Status variable for state if statement for calculating the dimensions
							} else { //Reset and do not increment rockCount
								status--;
								rck[rockCount].startIndex = 0;
							}
						} else if (rck[rockCount].endIndex == x /*&& LMS_measArray[x] <= 0*/ && rckStatus >= 1){
							rckStatus = 0;
							if(LMS_measArray[x] <= 0){
								//Rock ended, save in buffer to be sent
								do{
									if(rckBuf[rckControl].width <= 0){
										rckBuf[rckControl].width = rck[rockCount].width;
										rckBuf[rckControl].length = rck[rockCount].length;
										rckBuf[rckControl].height = rck[rockCount].height;
									}
									rckControl++;
								} while (rckControl < MAXROCKS);
								rckControl = 0;
							}
						}
						//-------------------^STILL UNDER DEVELOPMENT^------------------------------//
						//---------------------------------- calculate dimensions rock --------------------------------------------------//
						if(rockCount >= 1 && statusTwo == 1){
							//A status variable to check if the stone is present or not
							statusThree = 1;
							statusThreeCounter = 0;
							statusTwo = 0;

							//Axis Check
							xAxisControl = 0;
							xAxisStatus = 0;
							rckControl = 0;
							startIndexTemp = 0;
							endIndexTemp = 0;

							//width
							widthTemp = startWidth - endWidth;
							if(widthTemp > rck[rockCount-1].width){
								rck[rockCount-1].width = widthTemp;
							}
							//height
							heightTemp = LMS_dataArrayY[rck[rockCount-1].startIndex];
							for(int y = rck[rockCount-1].startIndex; y <= rck[rockCount-1].endIndex; y++){
								if(heightTemp > LMS_dataArrayY[y]){
									heightTemp = LMS_dataArrayY[y];
								}
							}
							heightTemp = BELT - heightTemp;
							if(heightTemp > rck[rockCount-1].height){
								rck[rockCount-1].height = heightTemp;
							}
							//length
							if(rck[rockCount-1].lengthS.lengthStep == 1){
								rck[rockCount-1].length = 0;
							} else if(rck[rockCount-1].lengthS.lengthStep > 1){
								rck[rockCount-1].length += ((SPEED)*((float)delta/1000000));
							}
						}

						/* index counter */
						if(LMS_measArray[x] != 0){
							indexCounter++;
						} else if (LMS_measArray[x] == 0 && indexCounter > 0){
							indexCounter = 0;
						}
					}
					if(statusThree == 0){
						statusThreeCounter++;
						//if after 30 data cycles no rocks are detected, reset rock values
						if(statusThreeCounter >= 30){
							bzero(&rck->width, sizeof(rck->width));
							bzero(&rck->height, sizeof(rck->height));
							bzero(&rck->length, sizeof(rck->length));
						}
					}
					statusThree = 0;
					rockCount = 0;
					angle = 0;
					xAxisRightShiftStatus = 0;
					xAxisLeftShiftStatus = 0;
				}
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

	char LMS_state = '0';
	unsigned char LMS_dataCounter = 0;
	float flashArr[4];

	unsigned int resolutionConv = 0;
	unsigned int freqConv = 0;
	unsigned int startAngleConv = 0;
	unsigned int endAngleConv = 0;

	unsigned int i = 0;
	char LMS_dataRawAmount[6] = {0, 0, 0, 0 ,0 ,0};
	unsigned short LMS_dataAmount = 0;
	char LMS_dataRaw[8] = {0, 0, 0, 0, 0 ,0 ,0 ,0};
	unsigned short LMS_measData[DATAPOINTMAX];
	unsigned short LMS_data = 0;
	unsigned short LMS_commaCounter = 0;
	unsigned int LMS_calibrationArray[DATAPOINTMAX];
	unsigned char thresholdStatus = 0;

	/* Infinite loop */
	for(;;)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
		vTaskDelay(1);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
		bzero(MEASdata_B, sizeof(MEASdata_B));

		lock = 1;
		reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
		if(output == 0 || output == 2 || output == 3){
			HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN); //Stress test run time
			HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
			sprintf(IOtrans_B, "%s,%.2f,%.2f,%.2f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u"
					",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
					NMEA_DATA, measTemp, measAmpMax, measFreq, humAlertOne, humAlertTwo, gTime.Hours, gTime.Minutes, gTime.Seconds,
					rck[0].height, rck[0].width, rck[0].length, rck[1].height, rck[1].width, rck[1].length,
					rck[2].height, rck[2].width, rck[2].length, rck[3].height, rck[3].width, rck[3].length,
					rck[4].height, rck[4].width, rck[4].length, rck[5].height, rck[5].width, rck[5].length,
					rck[6].height, rck[6].width, rck[6].length, rck[7].height, rck[7].width, rck[7].length,
					rck[8].height, rck[8].width, rck[8].length, rck[9].height, rck[9].width, rck[9].length);
			if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
				//error handler
				close(1);
				if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
					//error handler
					NVIC_SystemReset();
					for(;;);
				}
			}
			measTemp = 0; //package control in the HAL_UART_RxCpltCallback() function in main.c
		}

		if((retValIO = recvfrom(1, (uint8_t *)IOtrans_B, sizeof(IOtrans_B), (uint8_t *)IO_IP, 0)) < 0){
			//error handler
			NVIC_SystemReset();
			for(;;);
		}
		sscanf(IOtrans_B, "%[^,]", NMEArecv);
		if((strcmp(NMEArecv, "$PVRSCS") == 0)){
			sscanf(IOtrans_B, "%[^,],%d\r\n", NMEArecv, (int *)&tempSpeed);
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
			sscanf(IOtrans_B, "%[^,],%d\r\n", NMEArecv, (int *)&tempOutput);
			if(tempOutput != output){
				output = tempOutput;
				//Run once for timeout handler
				Flash_Write_NUM(FLASH_OUTPUT, output);
				//Save the values in the Flash
				Flash_Write_NUM(FLASH_OUTPUT, output);
			}
		} else if((strcmp(NMEArecv, "$PVRSCK") == 0)){
			//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
			for(int x = 0; x < INTERVAL; x++){
				//---------------------------------------------Get data from LMS-----------------------------------------------------------------//
				reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
				if(((retVal = socket(0, Sn_MR_TCP, 0, 0)) == 0)){
					if((retVal = connect(0, (uint8_t *)LMS_IP, 2111)) == SOCK_OK){ //Open socket with LMS
						sprintf((char *)LMStrans_B, "%c%s%c", 0x02, TELEGRAM_SCAN_ONE, 0x03); //Create telegram to be sent to LMS (Ask for one)
						reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
						if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
							//error handler
							NVIC_SystemReset();
							for(;;);
						}
						if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the telegram message from the LMS
							//error handler
							NVIC_SystemReset();
							for(;;);
						}
						disconnect(0);
						close(0);
						//---------------------------------------------Split Data-----------------------------------------------------------------//
						for(int j = 0; j < retVal; j++){
							if(LMSrecv_B[j] == 0x20){
								//Each data point counted after a comma
								LMS_commaCounter++;
							}
							if(LMS_commaCounter == 25){
								//on the 25th data point, the data amount can be found
								j++;
								while(LMSrecv_B[j] != 0x20){
									LMS_dataRawAmount[i++] = LMSrecv_B[j++];
								}
								j--;
								i = 0;
								LMS_dataAmount = strtol((char *)LMS_dataRawAmount, NULL, 16);
								bzero(LMS_dataRawAmount, sizeof(LMS_dataRawAmount));
							} else if(LMS_commaCounter == 26){ //convert all the data point to single integers in an array
								for(int x = 0; x < LMS_dataAmount; x++){
									j++;
									while(LMSrecv_B[j] != 0x20 && LMSrecv_B[j] != 0x0){
										LMS_dataRaw[i++] = LMSrecv_B[j++];
									}
									LMS_commaCounter++;
									i = 0;
									LMS_data = strtol((char *)LMS_dataRaw, NULL, 16);
									bzero(LMS_dataRaw, sizeof(LMS_dataRaw));
									LMS_measData[x] = LMS_data;
								}
								j--;
							} else if(LMSrecv_B[j] == 0x03){
								LMS_commaCounter = 0;
								break;
							}
						}
					}
				}
				if(thresholdStatus == 0){
					LMShighBelt_U = LMSlowBelt_U = LMS_measData[0];
					thresholdStatus++;
				}
				for(int y = 0; y < LMS_dataAmount; y++){
					LMS_calibrationArray[y] += LMS_measData[y];
					if(LMSlowBelt_U > LMS_measData[y])
						LMSlowBelt_U = LMS_measData[y];
					if(LMShighBelt_U < LMS_measData[y])
						LMShighBelt_U = LMS_measData[y];
				}
				for(int y = LMS_dataAmount; y < DATAPOINTMAX; y++){
					LMS_calibrationArray[y] = 0;
				}
			}
			for(int x = 0; x < LMS_dataAmount; x++){
				LMS_calibrationArray[x] = LMS_calibrationArray[x]/INTERVAL;
				LMSbelt_U += LMS_calibrationArray[x];
			}
			LMSbelt_U = LMSbelt_U/LMS_dataAmount;
			//flash the thresholds
			flashArr[0] = LMShighBelt_U;
			flashArr[1] = LMSlowBelt_U;
			flashArr[2] = LMSbelt_U;
			//Run once for timeout handler.
			Flash_Write_Data(FLASH_CALIBRATION, (uint32_t *)flashArr, 3);
			//Save the values in the Flash
			Flash_Write_Data(FLASH_CALIBRATION, (uint32_t *)flashArr, 3);
			//-------------------^STILL UNDER DEVELOPMENT^-----------------------------------//
		} else if((strcmp(NMEArecv, "$PVRSCC") == 0)){
			sprintf(IOtrans_B, "%s\r\n", update);
			if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
				//error handler
				close(1);
				if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
					//error handler
					NVIC_SystemReset();
					for(;;);
				}
			}
			sscanf(IOtrans_B, "%[^,],%d,%d,%f,%d\r\n", NMEArecv, &startAngle, &endAngle, &resolution, &freq);
			flashArr[0] = startAngle;
			flashArr[1] = endAngle;
			flashArr[2] = resolution;
			flashArr[3] = freq;
			//Run once for timeout handler.
			Flash_Write_Data(FLASH_PARAMETER, (uint32_t *)flashArr, sizeof(flashArr));
			//Save the values in the Flash
			Flash_Write_Data(FLASH_PARAMETER, (uint32_t *)flashArr, sizeof(flashArr));
			//configure 2D LiDAR sensor with the given parameters
			reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
			if((retVal = socket(0, Sn_MR_TCP, 0, 0)) == 0){
				if((retVal = connect(0, (uint8_t *)LMS_IP, 2111)) == SOCK_OK){ //Open socket with LMS
					sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_LOGIN, 0x03); //Create telegram to be sent to LMS (Ask for one data packet)
					if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
						//error handler
						for(;;);
					}
					if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
						//error handler
						for(;;);
					}

					/* configure setscan parameter LMS */
					freqConv = freq * 100;
					resolutionConv = resolution * 10000;
					sprintf(LMStrans_B, "%c%s %X %s %X %s%c", 0x02, TELEGRAM_SETSCAN_ONE, freqConv, TELEGRAM_SETSCAN_TWO, resolutionConv, TELEGRAM_SETSCAN_THREE, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
					if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
						//error handler
						for(;;);
					}
					if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
						//error handler
						for(;;);
					}

					/* configure output parameter LMS */
					startAngleConv = startAngle * 10000;
					endAngleConv = endAngle * 10000;
					sprintf(LMStrans_B, "%c%s %X %X%c", 0x02, TELEGRAM_OUTPUT_ONE, startAngleConv, endAngleConv, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
					if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
						//error handler
						for(;;);
					}
					if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
						//error handler
						for(;;);
					}

					sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_CONFIGURE_FOUR, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
					if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
						//error handler
						for(;;);
					}
					if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
						//error handler
						for(;;);
					}

					sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_STORE, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
					if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
						//error handler
						for(;;);
					}
					if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
						//error handler
						for(;;);
					}

					sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_LOGOUT, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
					if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
						//error handler
						for(;;);
					}
					if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
						//error handler
						for(;;);
					}

					sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_DEVICESTATE, 0x03);
					while(LMS_state != '1'){ //Wait till LMS sensor is done configuring
						HAL_IWDG_Refresh(&hiwdg1);
						if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
							//error handler
							for(;;);
						}
						if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
							//error handler
							for(;;);
						}
						for(int j = 0; j <= retVal-1; j++){
							if(LMSrecv_B[j] == 0x20){
								LMS_dataCounter++;
							}
							if(LMS_dataCounter == 2){
								j++;
								LMS_state = LMSrecv_B[j];
								LMS_dataCounter = 0;
								break;
							}
						}
					}
					LMS_state = '0';
					reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
					disconnect(0);
					close(0);
					reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
					close(1);
				}
			}
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
