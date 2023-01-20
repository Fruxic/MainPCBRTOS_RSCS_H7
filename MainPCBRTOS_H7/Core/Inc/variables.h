/*
 * variables.h
 * This is where all the Global variables are found used across the project
 *  Created on: Jan 17, 2023
 *      Author: Q6Q
 */

#ifndef INC_VARIABLES_H_
#define INC_VARIABLES_H_

extern const unsigned char LMS_IP[4];
extern const unsigned char IO_IP[4];
//
extern unsigned char LMS_recv[3000];
extern unsigned char LMS_buf[100];
//
extern unsigned char IO_recv[100];
extern unsigned char IO_buf[100];
//
extern unsigned char MEAS_data[50];
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

#endif /* INC_VARIABLES_H_ */
