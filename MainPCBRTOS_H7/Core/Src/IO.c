/*
 * IO.c
 *
 *  Created on: 6 Mar 2023
 *      Author: Q6Q
 */

//Includes
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "main.h"
#include "defines.h"
#include "variables.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "w5500_spi.h"
#include "LMS.h"
#include "IO.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern struct rock rck[MAXROCKS];
extern struct rockBuf rckBuf[MAXROCKS];

RTC_TimeTypeDef gTime;
RTC_DateTypeDef gDate;

/**
 ********************************************************************************
 * @brief Open socket of IO-server
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
unsigned char IO_openSocket(void){
	if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
		//error handler
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}

/**
 ********************************************************************************
 * @brief Open socket of IO-server
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
unsigned char IO_recv(void){
	if((retValIO = recvfrom(1, (uint8_t *)IOrecv_B, sizeof(IOrecv_B), (uint8_t *)IO_IP, 0)) < 0){
		//error handler
		return EXIT_SUCCESS;
		for(;;);
	} else {
		return EXIT_FAILURE;
	}
}

/**
 ********************************************************************************
 * @brief
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
unsigned char IO_sendPolarData(void){
	char strAppend[8];
	unsigned int strCount = 0;

	for(int x = 0; x <= VOCAM_MAX; x++){
		if(x < VOCAM_MAX){
			sprintf((char *)strAppend, "%d,", LMS_measData[x]);
			strCount += strlen(strAppend);
		} else if(x >= VOCAM_MAX){
			sprintf((char *)strAppend, "%d\r\n", LMS_measData[x]);
			strCount += strlen(strAppend);
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
		IO_close();
		if(IO_openSocket() == 1){
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
	}
	//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
	//					  if(strCount >= MTU){
	//						  sprintf(IOtrans_B, "%s", LMSpointCloudPolarTwo_B);
	//						  if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
	//							  //error handler
	//							  close(1);
	//							  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
	//								  //error handler
	//								  NVIC_SystemReset();
	//								  while(1);
	//							  }
	//						  }
	//						  if(strCount >= MTU*2){
	//							  sprintf(IOtrans_B, "%s", LMSpointCloudPolarThree_B);
	//							  if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
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
	return EXIT_SUCCESS;
}

/**
 ********************************************************************************
 * @brief
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
unsigned char IO_sendCartesianData(void){
	char strAppendX[8];
	char strAppendY[8];
	unsigned int strCountX = 0;
	unsigned int strCountY = 0;

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
		IO_close();
		if(IO_openSocket() == 1){
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
	}
	//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
	//					  if(strCountY >= MTU){
	//						  sprintf((char *)IOtrans_B, "%s", LMSpointCloudYTwo_B);
	//						  if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
	//							  //error handler
	//							  close(1);
	//							  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
	//								  //error handler
	//								  NVIC_SystemReset();
	//								  for(;;);
	//							  }
	//						  }
	//						  if(strCountY >= MTU*2){
	//							  sprintf((char *)IOtrans_B, "%s", LMSpointCloudYThree_B);
	//							  if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
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
		IO_close();
		if(IO_openSocket() == 1){
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
	}
	//-------------------STILL UNDER DEVELOPMENT-----------------------------------//
	//					  if(strCountX >= MTU){
	//						  sprintf(IOtrans_B, "%s", LMSpointCloudXTwo_B);
	//						  if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
	//							  //error handler
	//							  close(1);
	//							  if((retValIO = socket(1, Sn_MR_UDP, PORT, SF_IO_NONBLOCK)) != 1){
	//								  //error handler
	//								  NVIC_SystemReset();
	//								  for(;;);
	//							  }
	//						  }
	//						  if(strCountX >= MTU*2){
	//							  sprintf(IOtrans_B, "%s", LMSpointCloudXThree_B);
	//							  if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
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
	return EXIT_SUCCESS;
}

/**
 ********************************************************************************
 * @brief
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
unsigned char IO_sendMeasurementData(void){
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
		IO_close();
		if(IO_openSocket() == 1){
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
	}
	measTemp = 0; //package control in the HAL_UART_RxCpltCallback() function in main.c
	return EXIT_SUCCESS;
}

/**
 ********************************************************************************
 * @brief
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
unsigned char IO_sendConfiguredParameters(void){
	//Send the configured values to the VOCAM/IO-server
	sprintf(IOtrans_B, "%s,%d,%d,%f,%d,%d,%d\r\n", NMEA_FLASH, startAngle, endAngle, resolution, freq, output, 1);
	if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
		//error handler
		IO_close();
		if(IO_openSocket() == 1){
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
	} else {
		HAL_Delay(500);
		sprintf(IOtrans_B, "%s,%d,%d,%f,%d,%d,%d\r\n", NMEA_FLASH, startAngle, endAngle, resolution, freq, output, 0);
		if((retValIO = sendto(1, (uint8_t *)IOtrans_B, strlen(IOtrans_B), (uint8_t *)IO_IP, PORT)) < 0){
			//error handler
			IO_close();
			if(IO_openSocket() == 1){
				//error handler
				return EXIT_FAILURE;
				for(;;);
			}
		}
	}
	return EXIT_SUCCESS;
}

/**
 ********************************************************************************
 * @brief Used to close socket and/or to empty the buffer of the WIZCHIP
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
void IO_close(void){
	close(1);
}
