/*
 * variables.c
 * This is where all the Global variables are found used across the project
 *  Created on: Jan 17, 2023
 *      Author: Q6Q
 */
#include "defines.h"

const char LMS_IP[4] = {10, 16, 8, 100};
const char IO_IP[4] = {10, 16, 6, 213};
//
char LMS_recv[MTU];
char LMS_recvTwo[MTU];
char LMS_recvThree[MTU];
char LMS_recvTotal[MTU*3];
char LMS_buf[128];
//
char IO_recv[128];
char IO_recvTwo[128];
char IO_buf[2048];
//
char MEAS_data[21];
char LMS_pointCloudPolarOne[MTU];
char LMS_pointCloudXOne[MTU];
char LMS_pointCloudYOne[MTU];
char LMS_pointCloudPolarTwo[MTU];
char LMS_pointCloudXTwo[MTU];
char LMS_pointCloudYTwo[MTU];
char LMS_pointCloudPolarThree[MTU];
char LMS_pointCloudXThree[MTU];
char LMS_pointCloudYThree[MTU];
//
unsigned int LMS_highBelt = 0;
unsigned int LMS_lowBelt = 0;
unsigned int LMS_belt = 0;
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
unsigned int flashRead[4];
//
char lock = 0;
int output = 0;
int chipSelect = 0;
//
char update[] = "Configuring....";
char measError[] = "Measurement PCB not connected...";
