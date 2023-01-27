/*
 * variables.c
 * This is where all the Global variables are found used across the project
 *  Created on: Jan 17, 2023
 *      Author: Q6Q
 */
#include "defines.h"

const char LMS_IP[4] = {10, 16, 8, 100};
const char IO_IP[4] = {10, 16, 7, 213};
//
char LMS_recv[4096];
char LMS_buf[100];
//
char IO_recv[100];
char IO_buf[4096];
//
char MEAS_data[21];
char LMS_pointCloudPolar[4096];
char LMS_pointCloudX[4096];
char LMS_pointCloudY[4096];
//
int retValIO = 0;
int retVal = 0;
//
int humAlertOne = 0; //Alert hum
int humAlertTwo = 0;
float measAmpMax = 0;
float measFreq = 0;
float measTemp = 0;
//
unsigned int startAngle = 0;
unsigned int endAngle = 0;
float resolution = 0;
unsigned int freq = 0;
unsigned int speed = 0;
//
float flashRead[FLASH_ARRAYSIZE];
//
char lock = 0;
int output = 0;
//
char update[] = "Configuring....";
char measError[] = "Measurement PCB not connected...";
