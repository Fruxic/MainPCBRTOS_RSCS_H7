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
extern char LMSrecv_B[MTU];
extern char LMSrecvTwo_B[MTU];
extern char LMSrecvThree_B[MTU];
extern char LMSrecvTotal_B[MTU*3];
extern char LMStrans_B[128];
//
extern char IOrecv_B[128];
extern char IOrecvTwo_B[128];
extern char IOtrans_B[4096];
//
extern char MEASdata_B[22];
extern char LMSpointCloudPolarOne_B[2048];
extern char LMSpointCloudXOne_B[2048];
extern char LMSpointCloudYOne_B[2048];
extern char LMSpointCloudPolarTwo_B[2048];
extern char LMSpointCloudXTwo_B[2048];
extern char LMSpointCloudYTwo_B[2048];
extern char LMSpointCloudPolarThree_B[2048];
extern char LMSpointCloudXThree_B[2048];
extern char LMSpointCloudYThree_B[2048];
//
extern unsigned int LMShighBelt_U;
extern unsigned int LMSlowBelt_U;
extern unsigned int LMSbelt_U;
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
extern unsigned short LMS_measData[DATAPOINTMAX];
extern short LMS_calcDataX[DATAPOINTMAX];
extern unsigned short LMS_calcDataY[DATAPOINTMAX];
//
extern unsigned int startAngle;
extern unsigned int endAngle;
extern float resolution;
extern unsigned int freq;
extern unsigned int speed;
extern unsigned char device;
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
