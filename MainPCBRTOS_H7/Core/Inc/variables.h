/*
 * variables.h
 * This is where all the Global variables are found used across the project
 *  Created on: Jan 17, 2023
 *      Author: Q6Q
 */

#ifndef INC_VARIABLES_H_
#define INC_VARIABLES_H_

extern const char LMS_IP[4];
extern const char IO_IP[4];
//
extern char LMS_recv[4096];
extern char LMS_buf[100];
//
extern char IO_recv[100];
extern char IO_buf[4096];
//
extern char MEAS_data[21];
extern char LMS_pointCloudPolar[3000];
extern char LMS_pointCloudX[3000];
extern char LMS_pointCloudY[3000];
//
extern int retValIO;
extern int retVal;
//
extern int humAlertOne; //Alert hum
extern int humAlertTwo;
extern float measAmpMax;
extern float measFreq;
extern float measTemp;
//
extern unsigned int startAngle;
extern unsigned int endAngle;
extern float resolution;
extern unsigned int freq;
extern unsigned int speed;
//
extern float flashRead[FLASH_ARRAYSIZE];
//
extern char lock;
extern int output;
//
extern char update[];
extern char measError[];

#endif /* INC_VARIABLES_H_ */
