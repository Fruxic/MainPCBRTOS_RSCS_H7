/*
 * defines.h
 *	This is where all the defines are found used across the project
 *  Created on: Jan 17, 2023
 *      Author: Q6Q
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_

#define PI								3.142857

#define PORT							30189
#define NMEA_DATA						"$PVRSCD"
#define NMEA_POINT						"$PVRSCP"
#define NMEA_POINTX						"$PVRSCX"
#define NMEA_POINTY						"$PVRSCY"
#define NMEA_FLASH						"$PVRSCF"

#define TELEGRAM_LOGIN					"sMN SetAccessMode 04 81BE23AA"
#define TELEGRAM_SETSCAN_ONE			"sMN mLMPsetscancfg"
#define TELEGRAM_SETSCAN_TWO			"1"
#define TELEGRAM_SETSCAN_THREE			"FFF92230 225510"
#define TELEGRAM_OUTPUT_ONE				"sWN LMPoutputRange 1 9C4"
#define TELEGRAM_STORE					"sMN mEEwriteall"
#define TELEGRAM_LOGOUT					"sMN Run"
#define TELEGRAM_DEVICESTATE			"sRN SCdevicestate"
#define TELEGRAM_REBOOT					"sMN mSCreboot"
#define TELEGRAM_SCAN_ONE	 			"sRN LMDscandata"
#define TELEGRAM_DEVICENAME             "sRN DeviceIdent"
#define TELEGRAM_OUTPUTRANGE            "sRN LMPoutputRange"
#define TELEGRAM_FREQUENCY              "sRN LMPscancfg"
#define TELEGRAM_REBOOT                 "sMN mSCreboot"

//Flash memory allocations
#define FLASH_OUTPUT					0x08040000
#define FLASH_SPEED						0x08020000
#define FLASH_PARAMETER					0x08080000
#define FLASH_CALIBRATION				0x08060000

#define THRESHOLD						20 //in millimeters
#define MAXINDEXDIFF_X      			5
#define MAXHEIGHTDIFF_Y     			10  //in mm
#define MAXROCKS            			10
#define SPEED              				154 //in mm/s
#define DATAPOINTMAX					1024
#define MTU								1460
#define VOCAM_MAX						330
#define INTERVAL                        10

#define EXIT_SUCCESS					0
#define EXIT_FAILURE					1

#define SHT31_ADDR	0x44<<1

#endif /* INC_DEFINES_H_ */
