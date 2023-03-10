/*
 * IO.h
 *
 *  Created on: 6 Mar 2023
 *      Author: Q6Q
 */

#ifndef INC_IO_H_
#define INC_IO_H_

unsigned char IO_openSocket(void);

unsigned char IO_recv(void);

unsigned char IO_sendPolarData(void);

unsigned char IO_sendCartesianData(void);

unsigned char IO_sendMeasurementData(void);

unsigned char IO_sendConfiguredParameters(void);

void IO_close(void);

#endif /* INC_IO_H_ */
