/*
 * w5500_spi.h
 *
 *  Created on: 30 Jun 2022
 *      Author: Q6Q
 */

#ifndef INC_W5500_SPI_H_
#define INC_W5500_SPI_H_

void LMS_select(void);

void LMS_deselect(void);

void IO_select(void);

void IO_deselect(void);

void W5500Init();

#endif /* INC_W5500_SPI_H_ */
