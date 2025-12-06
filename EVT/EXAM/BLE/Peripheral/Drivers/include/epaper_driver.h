/********************************** (C) COPYRIGHT *******************************
 * File Name          : epaper_driver.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : E-Paper display driver header (2.9" 296x128)
 *******************************************************************************/

#ifndef EPAPER_DRIVER_H
#define EPAPER_DRIVER_H

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

// Display dimensions (after rotation to landscape)
#define EPAPER_WIDTH       296
#define EPAPER_HEIGHT      128

// Color definitions
#define EPAPER_WHITE       0xFF
#define EPAPER_BLACK       0x00

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize e-paper display driver
 */
extern void EPaper_Init(void);

/*
 * Show splash screen
 */
extern void EPaper_ShowSplash(void);

/*
 * Show full label with all information
 */
extern void EPaper_ShowFullLabel(FoodLabelState_t *state, const FoodItem_t *food);

/*
 * Update temperature display (partial refresh)
 */
extern void EPaper_UpdateTemperature(FoodLabelState_t *state);

/*
 * Update status indicator (partial refresh)
 */
extern void EPaper_UpdateStatus(FoodLabelState_t *state);

/*
 * Update product name with highlight (for edit mode)
 */
extern void EPaper_UpdateProductHighlight(FoodLabelState_t *state, const FoodItem_t *food, uint8_t highlighted);

/*
 * Update created date with highlight (for edit mode)
 */
extern void EPaper_UpdateCreatedDateHighlight(FoodLabelState_t *state, uint8_t highlighted);

/*
 * Update days left with highlight (for edit mode)
 */
extern void EPaper_UpdateDaysLeftHighlight(FoodLabelState_t *state, uint8_t highlighted);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* EPAPER_DRIVER_H */
