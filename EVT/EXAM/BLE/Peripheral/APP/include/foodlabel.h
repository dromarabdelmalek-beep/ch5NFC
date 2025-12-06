/********************************** (C) COPYRIGHT *******************************
 * File Name          : foodlabel.h
 * Author             : Converted from ESP32-C3 to CH583
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Food Label application header file
 *******************************************************************************/

#ifndef FOODLABEL_H
#define FOODLABEL_H

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

// PIN CONFIGURATION
#define LED_PIN            GPIO_Pin_8   // Status LED
#define WASTE_LED_PIN      GPIO_Pin_9   // Waste alert LED
#define BTN_FIELD          GPIO_Pin_0   // Field select button
#define BTN_VALUE_UP       GPIO_Pin_10  // Value increase button
#define BTN_VALUE_DOWN     GPIO_Pin_5   // Value decrease button

// E-Paper SPI Pins
#define EPAPER_CS_PIN      GPIO_Pin_4   // Chip Select
#define EPAPER_DC_PIN      GPIO_Pin_1   // Data/Command
#define EPAPER_RST_PIN     GPIO_Pin_2   // Reset
#define EPAPER_BUSY_PIN    GPIO_Pin_3   // Busy

// I2C Pins for SHT sensor
#define I2C_SDA_PIN        GPIO_Pin_6   // SDA
#define I2C_SCL_PIN        GPIO_Pin_7   // SCL

// Task Events
#define FOODLABEL_START_DEVICE_EVT      0x0001
#define FOODLABEL_SENSOR_READ_EVT       0x0002
#define FOODLABEL_BLE_UPDATE_EVT        0x0004
#define FOODLABEL_BUTTON_EVT            0x0008
#define FOODLABEL_WASTE_LED_EVT         0x0010
#define FOODLABEL_SPLASH_DONE_EVT       0x0020

/*********************************************************************
 * TYPEDEFS
 */

// Food item structure with temperature ranges
typedef struct
{
    const char* type;
    int daysToExpiry;
    float minTempF;  // Minimum safe temperature (°F)
    float maxTempF;  // Maximum safe temperature (°F)
} FoodItem_t;

// Application state structure
typedef struct
{
    // Product information
    int currentFoodIndex;

    // Temperature monitoring
    float lastStoredTemp;
    uint8_t coldChainBroken;

    // System
    uint32_t bootCount;

    // Current sensor readings
    float currentTemp;
    float currentHumidity;

    // Configuration (editable in edit mode)
    int configCreatedDay;
    int configCreatedMonth;
    int configCreatedYear;
    int configDaysLeft;
    int configExpiryDay;
    int configExpiryMonth;
    int configExpiryYear;

    // UI state
    uint8_t editMode;
    int8_t selectedField;  // -1 = no field selected
} FoodLabelState_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Food Label Application
 */
extern void FoodLabel_Init(void);

/*
 * Task Event Processor for the Food Label Application
 */
extern uint16_t FoodLabel_ProcessEvent(uint8_t task_id, uint16_t events);

/*
 * Build BLE advertisement data
 */
extern void buildAdvertData(void);

/*
 * Trigger button event from interrupt
 */
extern void FoodLabel_TriggerButtonEvent(void);

/*
 * Save application state
 */
extern void FoodLabel_SaveState(void);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* FOODLABEL_H */
