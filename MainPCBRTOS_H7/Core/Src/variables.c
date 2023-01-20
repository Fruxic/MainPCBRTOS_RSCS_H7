/*
 * variables.c
 * This is where all the Global variables are found used across the project
 *  Created on: Jan 17, 2023
 *      Author: Q6Q
 */

const unsigned char LMS_IP[4] = {10, 16, 8, 100};
const unsigned char IO_IP[4] = {10, 16, 7, 198};
//
unsigned char LMS_recv[3000];
unsigned char LMS_buf[100];
//
unsigned char IO_recv[100];
unsigned char IO_buf[100];
//
unsigned char MEAS_data[50];
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
float flashRead[4];
//
char lock = 0;
