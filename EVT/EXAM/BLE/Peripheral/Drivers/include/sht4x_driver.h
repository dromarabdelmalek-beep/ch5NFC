/********************************** (C) COPYRIGHT *******************************
 * File Name          : sht4x_driver.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : SHT4x temperature/humidity sensor driver header
 *******************************************************************************/

#ifndef SHT4X_DRIVER_H
#define SHT4X_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"

/*********************************************************************
 * CONSTANTS
 */

// SHT4x I2C address
#define SHT4X_I2C_ADDR     0x44

// SHT4x Commands
#define SHT4X_CMD_MEASURE_HIGH_PRECISION    0xFD
#define SHT4X_CMD_MEASURE_MED_PRECISION     0xF6
#define SHT4X_CMD_MEASURE_LOW_PRECISION     0xE0
#define SHT4X_CMD_SOFT_RESET                0x94

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize SHT4x sensor driver
 */
extern void SHT4x_Init(void);

/*
 * Read temperature and humidity from SHT4x
 * Returns TRUE on success, FALSE on failure
 */
extern uint8_t SHT4x_Read(float *tempC, float *humidity);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SHT4X_DRIVER_H */
