/********************************** (C) COPYRIGHT *******************************
 * File Name          : flash_storage.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Flash storage driver for persistent application state
 *******************************************************************************/

#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "foodlabel.h"

/*********************************************************************
 * CONSTANTS
 */

// Flash storage address (use upper flash area, 20KB from start)
#define FLASH_STORAGE_ADDR     (20 * 1024)

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize flash storage
 */
extern void FlashStorage_Init(void);

/*
 * Save application state to flash
 */
extern void FlashStorage_Save(FoodLabelState_t *state);

/*
 * Load application state from flash
 */
extern void FlashStorage_Load(FoodLabelState_t *state);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* FLASH_STORAGE_H */
