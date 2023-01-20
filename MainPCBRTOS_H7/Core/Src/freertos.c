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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId EmptyHandle;
osThreadId LMSHandle;
osThreadId HumidityHandle;
osThreadId IOHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void startEmpty(void const * argument);
void startLMS(void const * argument);
void startHumidity(void const * argument);
void startIO(void const * argument);

extern void MX_USB_DEVICE_Init(void);
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
  /* definition and creation of Empty */
  osThreadDef(Empty, startEmpty, osPriorityIdle, 0, 128);
  EmptyHandle = osThreadCreate(osThread(Empty), NULL);

  /* definition and creation of LMS */
  osThreadDef(LMS, startLMS, osPriorityRealtime, 0, 4096);
  LMSHandle = osThreadCreate(osThread(LMS), NULL);

  /* definition and creation of Humidity */
  osThreadDef(Humidity, startHumidity, osPriorityNormal, 0, 512);
  HumidityHandle = osThreadCreate(osThread(Humidity), NULL);

  /* definition and creation of IO */
  osThreadDef(IO, startIO, osPriorityAboveNormal, 0, 4096);
  IOHandle = osThreadCreate(osThread(IO), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_startEmpty */
/**
  * @brief  Function implementing the Empty thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_startEmpty */
void startEmpty(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN startEmpty */
//  //Read the flash for the parameters needed for the algorithm
//  Flash_Read_Data(FLASH_PARAMETER, (uint32_t *)flashRead, sizeof(flashRead));
//  startAngle = flashRead[0];
//  endAngle = flashRead[1];
//  resolution = flashRead[2];
//  freq = flashRead[3];
//  speed = Flash_Read_NUM(FLASH_SPEED);
  /* Infinite loop */
  for(;;)
  {
    osDelay(10);
  }
  // In case we accidentally leave loop
  osThreadTerminate(NULL);
  /* USER CODE END startEmpty */
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
	HAL_UART_Receive(&huart2, (uint8_t *)MEAS_data, sizeof(MEAS_data), 1000);
	sscanf((char *)MEAS_data, "%f,%f,%f,%d", &measAmpMax, &measFreq, &measTemp, &humAlertTwo);
	lock = 1;
	reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
	sprintf((char *)IO_buf, "%s,%.2f,%.2f,%.2f,%d,%d,%d\r\n", NMEA, measTemp, measAmpMax, measFreq, rck[0].height, rck[0].width, rck[0].length);
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
  /* USER CODE END startIO */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
