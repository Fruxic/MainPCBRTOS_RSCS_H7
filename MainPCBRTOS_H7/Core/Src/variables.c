/*
 * variables.c
 * This is where all the Global variables are found used across the project
 *  Created on: Jan 17, 2023
 *      Author: Q6Q
 */
#include "defines.h"

const char LMS_IP[4] = {192, 168, 200, 3};
const char IO_IP[4] = {10, 16, 6, 213};
//
char LMSrecv_B[MTU];
char LMSrecvTwo_B[MTU];
char LMSrecvThree_B[MTU];
char LMSrecvTotal_B[MTU*3];
char LMStrans_B[128];
//
char IOrecv_B[128];
char IOrecvTwo_B[128];
char IOtrans_B[4096];
//
char MEASdata_B[26];
char LMSpointCloudPolarOne_B[2048];
char LMSpointCloudXOne_B[2048];
char LMSpointCloudYOne_B[2048];
char LMSpointCloudPolarTwo_B[2048];
char LMSpointCloudXTwo_B[2048];
char LMSpointCloudYTwo_B[2048];
char LMSpointCloudPolarThree_B[2048];
char LMSpointCloudXThree_B[2048];
char LMSpointCloudYThree_B[2048];
//
unsigned int LMShighBelt_U = 0;
unsigned int LMSlowBelt_U = 0;
unsigned int LMSbelt_U = 0;
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
unsigned short LMS_measData[DATAPOINTMAX];
short LMS_calcDataX[DATAPOINTMAX];
unsigned short LMS_calcDataY[DATAPOINTMAX];
//
short startAngle = 0;
short endAngle = 0;
float resolution = 0;
unsigned int freq = 0;
unsigned int speed = 0;
unsigned char device = 0;
//
unsigned int flashRead[4];
//
char lock = 0;
int output = 0;
int chipSelect = 0;
//
char update[] = "Configuring....";
char measError[] = "Measurement PCB not connected...";
