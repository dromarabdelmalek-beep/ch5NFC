/********************************** (C) COPYRIGHT *******************************
 * File Name          : CONFIG.h
 * Author             : FoodLabel BLE Application
 * Version            : V1.0
 * Date               : 2025-12-06
 * Description        : BLE configuration for CH583 food label application
 *******************************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * CHIP Configuration
 */
#define CH58x                           1
#define CH583                           1

/*********************************************************************
 * E-PAPER Display Configuration
 */
// Select your e-paper display model:
// 1 = GDEY029T94 (Good Display 2.9", UC8151D controller) - RECOMMENDED
// 0 = Generic SSD1680 (2.9", SSD1680 controller)
#define EPAPER_GDEY029T94               1

/*********************************************************************
 * DEBUG Configuration
 */
#define DEBUG                           1

/*********************************************************************
 * BLE LIB Configuration
 */
#include "CH58xBLE_LIB.h"

/* BLE stack memory size (6KB for basic BLE peripheral) */
#define BLE_MEMHEAP_SIZE                (1024 * 6)

/* BLE MAC address source */
#define BLE_MAC                         TRUE

/* Connection parameters */
#define BLE_SNV                         DISABLE
#define CLK_OSC32K                      0   // 0: internal RC, 1: external crystal

/*********************************************************************
 * PERIPHERAL Role Configuration
 */
#define BLE_ROLE                        PERIPHERAL_ROLE

/*********************************************************************
 * HAL Configuration
 */
#define HAL_SLEEP                       TRUE   // Enable low power sleep mode
#define DCDC_ENABLE                     FALSE

#ifdef __cplusplus
}
#endif

#endif
