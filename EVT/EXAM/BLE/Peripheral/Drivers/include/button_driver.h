/********************************** (C) COPYRIGHT *******************************
 * File Name          : button_driver.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Button handling driver with GPIO interrupts
 *******************************************************************************/

#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

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

// Button press duration thresholds
#define BUTTON_DEBOUNCE_MS         300     // Debounce time
#define BUTTON_LONG_PRESS_MS       5000    // Long press duration (5 seconds)

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize button driver with GPIO interrupts
 */
extern void Buttons_Init(void);

/*
 * Handle button press event
 */
extern void Buttons_Handle(FoodLabelState_t *state, const FoodItem_t *foodDatabase, int numFoods);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_DRIVER_H */
