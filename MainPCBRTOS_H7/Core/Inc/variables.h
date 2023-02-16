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
extern char LMS_recv[MTU];
extern char LMS_recvTwo[MTU];
extern char LMS_recvThree[MTU];
extern char LMS_recvTotal[MTU*3];
extern char LMS_buf[128];
//
extern char IO_recv[128];
extern char IO_recvTwo[128];
extern char IO_buf[2048];
//
extern char MEAS_data[21];
extern char LMS_pointCloudPolarOne[MTU];
extern char LMS_pointCloudXOne[MTU];
extern char LMS_pointCloudYOne[MTU];
extern char LMS_pointCloudPolarTwo[MTU];
extern char LMS_pointCloudXTwo[MTU];
extern char LMS_pointCloudYTwo[MTU];
extern char LMS_pointCloudPolarThree[MTU];
extern char LMS_pointCloudXThree[MTU];
extern char LMS_pointCloudYThree[MTU];
//
extern unsigned int LMS_highBelt;
extern unsigned int LMS_lowBelt;
extern unsigned int LMS_belt;
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
extern float flashRead[4];
//
extern char lock;
extern int output;
extern int chipSelect;
//
extern char update[];
extern char measError[];

#endif /* INC_VARIABLES_H_ */
