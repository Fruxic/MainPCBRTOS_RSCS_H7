/*
 * LMS.h
 *
 *  Created on: 6 Mar 2023
 *      Author: Q6Q
 */

#ifndef INC_LMS_H_
#define INC_LMS_H_

//Structs
struct lengthPoint{
	unsigned short lengthStep;
	unsigned int ticks[500];
};

struct rock{
	unsigned short startIndex;
	unsigned short startIndexMax;
	unsigned short endIndex;
	unsigned short endIndexMax;
	struct lengthPoint lengthS;
	unsigned short width;
	unsigned short length;
	unsigned short height;
};

struct rockBuf{
	unsigned int width;
	unsigned int length;
	unsigned int height;
};

unsigned char LMS_connect(void);

void LMS_disconnect(void);

unsigned char LMS_getOneScan(void);

unsigned short LMS_dataSplit(void);

void LMS_algorithm(unsigned short LMS_dataAmount);

unsigned char LMS_getFrequencyResolution(void);

unsigned char LMS_getOutputRange(void);

unsigned char LMS_getDevice(void);

unsigned char LMS_configuration(void);

unsigned char LMS_calibration(void);

unsigned char LMS_reboot(void);

#endif /* INC_LMS_H_ */
