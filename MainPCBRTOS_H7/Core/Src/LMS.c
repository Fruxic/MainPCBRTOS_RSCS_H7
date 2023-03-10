/*
 * LMS.c
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
#include "FLASH_SECTOR_H7.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct rock rck[MAXROCKS];
struct rockBuf rckBuf[MAXROCKS];

extern RTC_TimeTypeDef gTime;
extern RTC_DateTypeDef gDate;

/**
 ********************************************************************************
 * @brief
 *
 * @param[in]  N/A
 * @param[out] N/A
 *
 * @retval
 *******************************************************************************/
unsigned char LMS_connect(void){
	if(((retVal = socket(0, Sn_MR_TCP, 0, 0)) == 0)){
		if((retVal = connect(0, (uint8_t *)LMS_IP, 2111)) == SOCK_OK){ //Open socket with LMS
			return EXIT_SUCCESS;
		} else {
			return EXIT_FAILURE;
		}
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
void LMS_disconnect(void){
	disconnect(0);
	close(0);
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
unsigned char LMS_getOneScan(void){
	if(LMS_connect() == EXIT_SUCCESS){
		sprintf((char *)LMStrans_B, "%c%s%c", 0x02, TELEGRAM_SCAN_ONE, 0x03); //Create telegram to be sent to LMS (Ask for one)
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			LMS_disconnect();
			return EXIT_FAILURE;
			//			NVIC_SystemReset();
			for(;;);
		} else {
			if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the telegram message from the LMS
				//error handler
				LMS_disconnect();
				return EXIT_FAILURE;
				//				NVIC_SystemReset();
				for(;;);
			}
		}
		LMS_disconnect();
		return EXIT_SUCCESS;
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
unsigned short LMS_dataSplit(void){
	unsigned short LMS_spaceCounter = 0;                        //
	unsigned short LMS_dataAmount = 0;							//
	char LMS_dataRawAmount[6] = {0, 0, 0, 0, 0, 0};     		//
	char LMS_dataRaw[8] = {0, 0, 0, 0, 0, 0, 0, 0};    			//
	unsigned short LMS_data = 0;                                //
	unsigned char i = 0;

	for(int j = 0; j < retVal; j++){
		if(LMSrecv_B[j] == 0x20){
			//Each data point counted after a comma
			LMS_spaceCounter++;
		}
		if(LMS_spaceCounter == 25){
			//on the 25th data point, the data amount can be found
			j++;
			while(LMSrecv_B[j] != 0x20){
				LMS_dataRawAmount[i++] = LMSrecv_B[j++];
			}
			j--;
			i = 0;
			LMS_dataAmount = strtol((char *)LMS_dataRawAmount, NULL, 16);
			bzero(LMS_dataRawAmount, sizeof(LMS_dataRawAmount));
		} else if(LMS_spaceCounter == 26){ //convert all the data point to single integers in an array
			for(int x = 0; x < LMS_dataAmount; x++){
				j++;
				while(LMSrecv_B[j] != 0x20 && LMSrecv_B[j] != 0x0){
					LMS_dataRaw[i++] = LMSrecv_B[j++];
				}
				LMS_spaceCounter++;
				i = 0;
				LMS_data = strtol((char *)LMS_dataRaw, NULL, 16);
				bzero(LMS_dataRaw, sizeof(LMS_dataRaw));
				LMS_measData[x] = LMS_data;
			}
			j--;
		} else if(LMSrecv_B[j] == 0x03){
			LMS_spaceCounter = 0;
			break;
		}
	}
	return LMS_dataAmount;
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
void LMS_algorithm(unsigned short LMS_dataAmount){
	unsigned short LMS_measArray[DATAPOINTMAX];

	unsigned char status = 0;
	unsigned char statusTwo = 0;
	unsigned char statusThree = 0;
	static unsigned char statusThreeCounter = 0;
	unsigned short indexCounter = 0;
	unsigned char rockCount = 0;
	short LMS_dataArrayX[DATAPOINTMAX];
	unsigned short LMS_dataArrayY[DATAPOINTMAX];
	int startWidth = 0;
	int endWidth = 0;
	unsigned int heightTemp = 0;
	unsigned int widthTemp = 0;

	float angle;

	unsigned char xAxisControl = 0;
	unsigned char xAxisStatus = 0;
	unsigned char rckControl = 0;
	unsigned char xAxisRightShiftStatus = 0;
	unsigned char xAxisLeftShiftStatus = 0;
	unsigned short startIndexTemp = 0;
	unsigned short endIndexTemp = 0;
	unsigned char rckStatus = 0;

	unsigned int start = 0;
	unsigned int stop = 0;
	unsigned int delta = 0;

	static unsigned char emptySecDelta = 0;
	static unsigned char emptySecStart = 0;

	stop  = HAL_GetTick();
	delta = stop - start; //Measure time interval
	start = HAL_GetTick();

	angle = startAngle;
	for(int x = 0; x < LMS_dataAmount; x++){
		//From Polar coordinates to Cartesian coordinates.
		LMS_calcDataX[x] = LMS_measData[x]*cos((angle*PI)/180);
		LMS_calcDataY[x] = LMS_measData[x]*sin((angle*PI)/180);
		angle = angle + resolution;
		if(LMS_calcDataY[x] < LMSlowBelt_U - THRESHOLD){
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
			heightTemp = LMSbelt_U - heightTemp;
			if(heightTemp > rck[rockCount-1].height){
				rck[rockCount-1].height = heightTemp;
			}
			//length
			if(rck[rockCount-1].lengthS.lengthStep == 1){
				rck[rockCount-1].length = 0;
			} else if(rck[rockCount-1].lengthS.lengthStep > 1){
				rck[rockCount-1].length += ((speed)*((float)delta/1000000));
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
		if(statusThreeCounter == 0){
			HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
			emptySecStart = gTime.Seconds;
			statusThreeCounter++;
		} else {
			HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
			if(gTime.Seconds > emptySecStart){
				emptySecDelta += gTime.Seconds - emptySecStart;
				emptySecStart = gTime.Seconds;
			} else if(gTime.Seconds < emptySecStart){
				emptySecDelta += (gTime.Seconds - emptySecStart) + 60;
				emptySecStart = gTime.Seconds;
			}
			//if after 30 data cycles no rocks are detected, reset rock values
			if(emptySecDelta >= 30){
				for(int x = 0; x < 10; x++){
					rck[x].width = 0;
					rck[x].height = 0;
					rck[x].length = 0;
				}
				statusThreeCounter = 0;
				emptySecDelta = 0;
			}
		}
	}
	statusThree = 0;
	rockCount = 0;
	angle = 0;
	xAxisRightShiftStatus = 0;
	xAxisLeftShiftStatus = 0;
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
unsigned char LMS_getFrequencyResolution(void){
	unsigned short LMS_spaceCounter = 0;
	char LMS_rawResolution[6] = {0, 0, 0, 0, 0, 0};
	char LMS_rawFrequency[6] = {0, 0, 0, 0, 0, 0};
	unsigned char i = 0;

	if(LMS_connect() == EXIT_SUCCESS){
		sprintf((char *)LMStrans_B, "%c%s%c", 0x02, TELEGRAM_FREQUENCY, 0x03); //Create telegram to be sent to LMS
		reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) < 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			//			NVIC_SystemReset();
			for(;;);
		} else {
			if((retVal = recv(0, (uint8_t *)LMSrecvTotal_B, sizeof(LMSrecvTotal_B))) <= 0){ //Receive the telegram message from the LMS
				//error handler
				return EXIT_FAILURE;
				//				NVIC_SystemReset();
				for(;;);
			}
		}
		LMS_disconnect();

		for(int j = 0; j < retVal; j++){
			if(LMSrecvTotal_B[j] == 0x20){ //enter this function if space is read from received data of LMS
				LMS_spaceCounter++; //Each data point counted after a space
			}
			if(LMS_spaceCounter == 2){
				j++; //increment for loop variable to read the the byte containing the data amount and not the space
				while(LMSrecvTotal_B[j] != 0x20){ //keep reading the data until space is found.
					LMS_rawFrequency[i++] = LMSrecvTotal_B[j++];
				}
				j--; //decrement for loop variable
				i = 0; //reset to 0 for next data read.
				freq = strtol((char *)LMS_rawFrequency, NULL, 16) / 100; //convert from HEX to DEC
			} else if(LMS_spaceCounter == 4){
				j++; //increment for loop variable to read the the byte containing the data amount and not the space
				while(LMSrecvTotal_B[j] != 0x20){ //keep reading the data until space is found.
					LMS_rawResolution[i++] = LMSrecvTotal_B[j++];
				}
				j--; //decrement for loop variable
				i = 0; //reset to 0 for next data read.
				resolution = (float)strtol((char *)LMS_rawResolution, NULL, 16) / 10000; //convert from HEX to DEC
				break;
			}
		}
		return EXIT_SUCCESS;
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
unsigned char LMS_getOutputRange(void){
	unsigned short LMS_spaceCounter = 0;
	char LMS_rawStartAngle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	char LMS_rawEndAngle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char i = 0;

	if(LMS_connect() == EXIT_SUCCESS){
		sprintf((char *)LMStrans_B, "%c%s%c", 0x02, TELEGRAM_OUTPUTRANGE, 0x03); //Create telegram to be sent to LMS (Ask for one Poll)
		reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			//			NVIC_SystemReset();
			for(;;);
		} else {
			if((retVal = recv(0, (uint8_t *)LMSrecvTotal_B, sizeof(LMSrecvTotal_B))) <= 0){ //Receive the telegram message from the LMS
				//error handler
				return EXIT_FAILURE;
				//				NVIC_SystemReset();
				for(;;);
			}
		}
		LMS_disconnect();

		for(int j = 0; j < retVal; j++){
			if(LMSrecvTotal_B[j] == 0x20){ //enter this function if space is read from received data of LMS
				LMS_spaceCounter++; //Each data point counted after a space
			}
			if(LMS_spaceCounter == 4){
				j++; //increment for loop variable to read the the byte containing the data amount and not the space
				while(LMSrecvTotal_B[j] != 0x20){ //keep reading the data until space is found.
					LMS_rawStartAngle[i++] = LMSrecvTotal_B[j++];
				}
				j--; //decrement for loop variable
				i = 0; //reset to 0 for next data read.
				startAngle = strtol((char *)LMS_rawStartAngle, NULL, 16) / 10000; //convert from HEX to DEC
			} else if(LMS_spaceCounter == 5){
				j++; //increment for loop variable to read the the byte containing the data amount and not the space
				while(LMSrecvTotal_B[j] != 0x03){ //keep reading the data until TELEGRAM end character is found.
					LMS_rawEndAngle[i++] = LMSrecvTotal_B[j++];
				}
				j--; //decrement for loop variable
				i = 0; //reset to 0 for next data read.
				endAngle = strtol((char *)LMS_rawEndAngle, NULL, 16) / 10000; //convert from HEX to DEC
				break;
			}
		}
		return EXIT_SUCCESS;
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
unsigned char LMS_getDevice(void){
	unsigned short LMS_spaceCounter = 0;
	unsigned char i = 0;

	char LMS_rawDeviceName[10];
	bzero(LMS_rawDeviceName, sizeof(LMS_rawDeviceName));

	if(LMS_connect() == EXIT_SUCCESS){
		sprintf((char *)LMStrans_B, "%c%s%c", 0x02, TELEGRAM_DEVICENAME, 0x03); //Create telegram to be sent to LMS (Ask for one Poll)
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		} else {
			if((retVal = recv(0, (uint8_t *)LMSrecvTotal_B, sizeof(LMSrecvTotal_B))) <= 0){ //Receive the telegram message from the LMS
				//error handler
				return EXIT_FAILURE;
				for(;;);
			}
		}
		LMS_disconnect();

		for(int j = 0; j < retVal; j++){
			if(LMStrans_B[j] == 0x20){ //enter this function if space is read from received data of LMS
				LMS_spaceCounter++; //Each data point counted after a space
			}
			if(LMS_spaceCounter == 3){
				j++; //increment for loop variable to read the the byte containing the data amount and not the space
				while(i < 5){ //keep reading the data until space is found.
					LMS_rawDeviceName[i++] = LMStrans_B[j++];
				}
				j--; //decrement for loop variable
				i = 0; //reset to 0 for next data read.
				if((strcmp((char *)LMS_rawDeviceName, "LMS_1") == 0)){
					device = 0;
				} else if((strcmp((char *)LMS_rawDeviceName, "LMS_5") == 0)){
					device = 1;
				}
				break;
			}
		}
		return EXIT_SUCCESS;
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
unsigned char LMS_configuration(void){
	char NMEArecv[7];
	char LMS_state = '0';
	unsigned char LMS_dataCounter = 0;

	unsigned int resolutionConv = 0;
	unsigned int freqConv = 0;
	unsigned int startAngleConv = 0;
	unsigned int endAngleConv = 0;

	unsigned char failedStateMinutesDelta = 0;
	unsigned char failedStateMinutesStart = 0;

	unsigned short angleBuf;

	sscanf(IOrecv_B, "%[^,],%d,%d,%f,%d\r\n", NMEArecv, &startAngle, &endAngle, &resolution, &freq);
	if(endAngle < startAngle){
		angleBuf = startAngle;
		startAngle = endAngle;
		endAngle = angleBuf;
	}
	//configure 2D LiDAR sensor with the given parameters
	if(LMS_connect() == EXIT_SUCCESS){
		sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_LOGIN, 0x03); //Create telegram to be sent to LMS (Ask for one data packet)
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		/* configure setscan parameter LMS */
		freqConv = freq * 100;
		resolutionConv = resolution * 10000;
		sprintf(LMStrans_B, "%c%s %X %s %X %s%c", 0x02, TELEGRAM_SETSCAN_ONE, freqConv, TELEGRAM_SETSCAN_TWO, resolutionConv, TELEGRAM_SETSCAN_THREE, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		/* configure output parameter LMS */
		startAngleConv = startAngle * 10000;
		endAngleConv = endAngle * 10000;
		sprintf(LMStrans_B, "%c%s %X %X%c", 0x02, TELEGRAM_OUTPUT_ONE, startAngleConv, endAngleConv, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		/* store parameter in LMS */
		sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_STORE, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
			//error handler
			for(;;);
		}

		sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_LOGOUT, 0x03); //Create telegram to be sent to LMS (Ask for one datapacket)
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
			//error handler
			return EXIT_FAILURE;
			for(;;);
		}
		sprintf(LMStrans_B, "%c%s%c", 0x02, TELEGRAM_DEVICESTATE, 0x03);
		HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
		failedStateMinutesStart = gTime.Minutes;
		failedStateMinutesDelta = 0;
		while(LMS_state != '1'){ //Wait till LMS sensor is done configuring
			HAL_IWDG_Refresh(&hiwdg1);
			HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
			if(gTime.Minutes > failedStateMinutesStart){
				failedStateMinutesDelta += gTime.Minutes - failedStateMinutesStart;
				failedStateMinutesStart = gTime.Minutes;
			} else if(gTime.Minutes < failedStateMinutesStart){
				failedStateMinutesDelta += (gTime.Minutes - failedStateMinutesStart) + 60;
				failedStateMinutesStart = gTime.Minutes;
			}
			if(failedStateMinutesDelta >= 2){
				failedStateMinutesDelta = 0;
				LMS_reboot();
			} else {
				if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
					//error handler
					for(;;);
				}
				if((retVal = recv(0, (uint8_t *)LMSrecv_B, sizeof(LMSrecv_B))) <= 0){ //Receive the data packet from the LMS
					//error handler
					for(;;);
				}
				for(int j = 0; j < retVal; j++){
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
		}
		LMS_state = '0';
		LMS_disconnect();
		return EXIT_SUCCESS;
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
unsigned char LMS_calibration(void){
	float flashArr[4];
	float angle;
	unsigned short LMS_dataAmount = 0;
	unsigned int LMS_calibrationArray[DATAPOINTMAX];
	unsigned char thresholdStatus = 0;
	unsigned char skippedScan = 0;

	bzero(LMS_calibrationArray, sizeof(LMS_calibrationArray));
	for(int x = 0; x < INTERVAL; x++){
		if(skippedScan >= 3){
			LMSlowBelt_U = LMShighBelt_U = LMSbelt_U = 0;
			return EXIT_FAILURE;
		}
		if(LMS_getOneScan() == EXIT_SUCCESS){
			LMS_dataAmount = LMS_dataSplit();
			angle = startAngle;
			for(int x = 0; x < LMS_dataAmount; x++){
				//From Polar coordinates to Cartesian coordinates.
				LMS_calcDataY[x] = LMS_measData[x]*sin((angle*PI)/180);
				angle = angle + resolution;
			}
			if(thresholdStatus == 0){
				LMShighBelt_U = LMSlowBelt_U = LMS_calcDataY[0];
				thresholdStatus++;
			}
			for(int y = 0; y < LMS_dataAmount; y++){
				LMS_calibrationArray[y] += LMS_calcDataY[y];
				if(LMSlowBelt_U > LMS_calcDataY[y])
					LMSlowBelt_U = LMS_calcDataY[y];
				if(LMShighBelt_U < LMS_calcDataY[y])
					LMShighBelt_U = LMS_calcDataY[y];
			}
			for(int y = LMS_dataAmount; y < DATAPOINTMAX; y++){
				LMS_calibrationArray[y] = 0;
			}
		} else {
			skippedScan++;
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
unsigned char LMS_reboot(void){
	if(LMS_connect() == EXIT_SUCCESS){
		sprintf((char *)LMStrans_B, "%c%s%c", 0x02, TELEGRAM_REBOOT, 0x03); //Create telegram to be sent to LMS
		if((retVal = send(0, (uint8_t *)LMStrans_B, strlen(LMStrans_B))) <= 0){ //Send the Telegram to LMS
			//error handler
			return EXIT_FAILURE;
			//			NVIC_SystemReset();
			for(;;);
		} else {
			if((retVal = recv(0, (uint8_t *)LMSrecvTotal_B, sizeof(LMSrecvTotal_B))) <= 0){ //Receive the telegram message from the LMS
				//error handler
				return EXIT_FAILURE;
				//				NVIC_SystemReset();
				for(;;);
			}
		}
		LMS_disconnect();
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
