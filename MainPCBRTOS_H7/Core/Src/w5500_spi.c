/*
 * w5500_spi.c
 *
 *  Created on: 30 Jun 2022
 *      Author: Q6Q
 */

/*includes*/
#include "stm32h7xx_hal.h"
#include "wizchip_conf.h"
#include "stdio.h"

/*Type Defines*/
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi4;
//extern IWDG_HandleTypeDef hiwdg1;

/*Defines*/
#define TRUE 1
#define FALSE 0

uint8_t LMS_status = FALSE;
uint8_t IO_status = FALSE;

/*Functions*/
void LMS_select(void){
	LMS_status = TRUE;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); //Makes CS pin LOW
}

void LMS_deselect(void){
	LMS_status = FALSE;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); //Makes CS pin HIGH
}

void IO_select(void){
	IO_status = TRUE;
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET); //Makes CS pin LOW
}

void IO_deselect(void){
	IO_status = FALSE;
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET); //Makes CS pin HIGH
}

//Pointers has to be added here to differentiate from both received data packages
uint8_t SPIRead(void){
	uint8_t rb = 0;
	if(LMS_status == TRUE){
		HAL_SPI_Receive(&hspi1, &rb, 1, HAL_MAX_DELAY);
	}else if(IO_status == TRUE){
		HAL_SPI_Receive(&hspi4, &rb, 1, HAL_MAX_DELAY);
	}
	return rb;
}

//Pointers has to be added here to differentiate from both transmitted data packages
void SPIWrite(uint8_t wb){
	if(LMS_status == TRUE){
		HAL_SPI_Transmit(&hspi1, &wb, 1, HAL_MAX_DELAY);
	}else if(IO_status == TRUE){
		HAL_SPI_Transmit(&hspi4, &wb, 1, HAL_MAX_DELAY);
	}
}

uint8_t wizchip_read(void){
	uint8_t readByte;
	readByte = SPIRead();
	return readByte;
}

void wizchip_write(uint8_t writeByte){
	SPIWrite(writeByte);
}

void wizchip_readBurst(uint8_t* pBuf, uint16_t len){
	for(uint16_t i = 0; i < len; i++){
		*pBuf = SPIRead();
		pBuf++;
	}
}

void wizchip_writeBurst(uint8_t *pBuf, uint16_t len){
	for(uint16_t i = 0; i < len; i++){
		SPIWrite(*pBuf);
		pBuf++;
	}
}

void LMS_settings(void){
	  wiz_NetInfo netInfo = { .mac 	= {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},	// Mac address
	                          .ip 	= {192, 168, 200, 1},					// IP address
	                          .sn 	= {255, 255, 255, 0},					// Subnet mask
	                          .gw 	= {192, 168, 200, 0}};				    	// Gateway address
	  wizchip_setnetinfo(&netInfo);
}

void IO_settings(void){
	  wiz_NetInfo netInfo = { .mac 	= {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},	// Mac address
	                          .ip 	= {10, 16, 6, 189},					    // IP address
	                          .sn 	= {255, 255, 255, 128},					// Subnet mask
	                          .gw 	= {10, 16, 6, 0}};				    	// Gateway address
	  wizchip_setnetinfo(&netInfo);

//	  /* Set interrupt */
//	  setSn_IMR(1, Sn_IR_RECV);
//	  setSIMR(0x02);
//	  setSIR(0x00);
}

void W5500Init(){
	uint8_t tmp;
	uint8_t memsize[2][8]= {{4, 4, 0, 0, 0, 0, 0, 0},{4, 4, 0, 0, 0, 0, 0, 0}};

	LMS_deselect(); //CS HIGH by default
	IO_deselect(); //CS HIGH by default

	//Reset both the W5500 chip
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
	tmp = 0xFF;
	while(tmp--);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
	reg_wizchip_spiburst_cbfunc(wizchip_readBurst, wizchip_writeBurst);

	/* Initialise first WIZchip (LMS connection) on RSCS */
	reg_wizchip_cs_cbfunc(LMS_select, LMS_deselect);
	if(ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize) == -1){
		while(1); // Initialisation failed!
	}
	/*Initialise MAC, IP, and the rest for LMS here*/
	LMS_settings();

	/* Initialise second WIZchip (IO connection) on RSCS */
	reg_wizchip_cs_cbfunc(IO_select, IO_deselect);
	if(ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize) == -1){
		while(1); // Initialisation failed!
	}
	/*Initialise MAC, IP, and the rest for IO here*/
	IO_settings();
}
