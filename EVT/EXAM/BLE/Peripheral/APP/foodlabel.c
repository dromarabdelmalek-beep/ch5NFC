/********************************** (C) COPYRIGHT *******************************
 * File Name          : foodlabel.c
 * Author             : Converted from ESP32-C3 to CH583
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Food Label application logic with cold chain monitoring
 *                      and BLE broadcast
 *******************************************************************************/

#include "CONFIG.h"
#include "foodlabel.h"
#include "gattprofile.h"
#include "epaper_driver.h"
#include "sht4x_driver.h"
#include "flash_storage.h"
#include "button_driver.h"

/*********************************************************************
 * MACROS
 */
// How often to perform periodic sensor reading (10 minutes = 600000 ms)
#define SENSOR_READ_PERIOD                  600000  // 10 minutes in ms for TMOS

// Temperature threshold for display update (°F)
#define TEMP_THRESHOLD                      2.0f

// Waste LED blink interval (5 minutes)
#define WASTE_LED_BLINK_PERIOD              300000

// How often to update BLE advertisement data (1 minute)
#define BLE_UPDATE_PERIOD                   60000

// Advertising interval (units of 625us, 80=50ms)
#define DEFAULT_ADVERTISING_INTERVAL        80

// Company Identifier: WCH
#define WCH_COMPANY_ID                      0x07D7

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t FoodLabel_TaskID = INVALID_TASK_ID;

// Food Database with temperature ranges
const FoodItem_t foodDatabase[] = {
    {"CHICKEN",     7,  32.0, 40.0},  // 0-4°C
    {"BEEF",        5,  32.0, 40.0},  // 0-4°C
    {"FISH",        3,  30.0, 34.0},  // -1-1°C (colder than meat)
    {"PORK",        6,  32.0, 40.0},  // 0-4°C
    {"SEAFOOD",     2,  30.0, 34.0},  // -1-1°C
    {"VEGETABLES", 10,  32.0, 50.0},  // 0-10°C
    {"DAIRY",       4,  32.0, 40.0},  // 0-4°C
    {"FRUITS",      8,  32.0, 45.0}   // 0-7°C
};

const int NUM_FOODS = sizeof(foodDatabase) / sizeof(FoodItem_t);

// Application state (stored in flash, loaded on boot)
FoodLabelState_t appState = {
    .currentFoodIndex = 0,
    .lastStoredTemp = 77.0,
    .coldChainBroken = FALSE,
    .bootCount = 0,
    .currentTemp = 77.0,
    .currentHumidity = 0.0,
    .configCreatedDay = 5,
    .configCreatedMonth = 12,
    .configCreatedYear = 25,
    .configDaysLeft = 7,
    .configExpiryDay = 12,
    .configExpiryMonth = 12,
    .configExpiryYear = 25,
    .editMode = FALSE,
    .selectedField = -1
};

// BLE Advertisement data
static uint8_t advertData[31];
static uint8_t scanRspData[31];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void FoodLabel_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void foodLabelStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void performSensorRead(void);
static void updateBLEAdvertisement(void);
static void calculateExpiryDate(void);
static void checkColdChain(void);
static void blinkWasteLED(void);
static void updateDisplay(uint8_t fullRefresh);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t FoodLabel_PeripheralCBs = {
    foodLabelStateNotificationCB,  // Profile State Change Callbacks
    NULL,                          // RSSI callback (not used)
    NULL                           // Param update callback (not used)
};

// GAP Bond Manager Callbacks (not used for beacon-only mode)
static gapBondCBs_t FoodLabel_BondMgrCBs = {
    NULL,  // Passcode callback
    NULL,  // Pairing state callback
    NULL   // OOB callback
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      FoodLabel_Init
 *
 * @brief   Initialization function for Food Label application
 *
 * @return  none
 */
void FoodLabel_Init()
{
    // Register task with TMOS
    FoodLabel_TaskID = TMOS_ProcessEventRegister(FoodLabel_ProcessEvent);

    PRINT("Food Label Init\n");

    // Initialize peripherals
    EPaper_Init();
    SHT4x_Init();
    FlashStorage_Init();
    Buttons_Init();

    // Load state from flash
    FlashStorage_Load(&appState);
    appState.bootCount++;

    PRINT("Boot count: %d\n", appState.bootCount);

    // Calculate expiry date
    calculateExpiryDate();

    // Setup BLE Advertisement
    {
        uint8_t  initial_advertising_enable = TRUE;

        // Build manufacturer-specific advertisement data
        buildAdvertData();

        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
    }

    // Set advertising interval
    {
        uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, advInt);
    }

    // Display splash screen on first boot
    if(appState.bootCount == 1)
    {
        EPaper_ShowSplash();
        tmos_start_task(FoodLabel_TaskID, FOODLABEL_SPLASH_DONE_EVT, 3200); // 3.2 sec delay
    }
    else
    {
        // Read sensor and update display
        performSensorRead();
    }

    // Start the GAP Role peripheral
    tmos_set_event(FoodLabel_TaskID, FOODLABEL_START_DEVICE_EVT);

    // Schedule periodic sensor reading (10 minutes)
    tmos_start_task(FoodLabel_TaskID, FOODLABEL_SENSOR_READ_EVT, MS1_TO_SYSTEM_TIME(SENSOR_READ_PERIOD));

    // Schedule BLE update (1 minute)
    tmos_start_task(FoodLabel_TaskID, FOODLABEL_BLE_UPDATE_EVT, MS1_TO_SYSTEM_TIME(BLE_UPDATE_PERIOD));
}

/*********************************************************************
 * @fn      FoodLabel_ProcessEvent
 *
 * @brief   Food Label Application Task event processor
 *
 * @param   task_id - The TMOS assigned task ID
 * @param   events - events to process
 *
 * @return  events not processed
 */
uint16_t FoodLabel_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(FoodLabel_TaskID)) != NULL)
        {
            FoodLabel_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
            tmos_msg_deallocate(pMsg);
        }

        return (events ^ SYS_EVENT_MSG);
    }

    if(events & FOODLABEL_START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_PeripheralStartDevice(FoodLabel_TaskID, &FoodLabel_BondMgrCBs, &FoodLabel_PeripheralCBs);
        return (events ^ FOODLABEL_START_DEVICE_EVT);
    }

    if(events & FOODLABEL_SPLASH_DONE_EVT)
    {
        // Splash screen done, show full label
        EPaper_ShowFullLabel(&appState, &foodDatabase[appState.currentFoodIndex]);
        performSensorRead();
        return (events ^ FOODLABEL_SPLASH_DONE_EVT);
    }

    if(events & FOODLABEL_SENSOR_READ_EVT)
    {
        // Read sensor
        performSensorRead();

        // Restart timer for next reading (10 minutes)
        tmos_start_task(FoodLabel_TaskID, FOODLABEL_SENSOR_READ_EVT, MS1_TO_SYSTEM_TIME(SENSOR_READ_PERIOD));

        return (events ^ FOODLABEL_SENSOR_READ_EVT);
    }

    if(events & FOODLABEL_BLE_UPDATE_EVT)
    {
        // Update BLE advertisement data
        updateBLEAdvertisement();

        // Restart timer for next update (1 minute)
        tmos_start_task(FoodLabel_TaskID, FOODLABEL_BLE_UPDATE_EVT, MS1_TO_SYSTEM_TIME(BLE_UPDATE_PERIOD));

        return (events ^ FOODLABEL_BLE_UPDATE_EVT);
    }

    if(events & FOODLABEL_BUTTON_EVT)
    {
        // Handle button press
        Buttons_Handle(&appState, &foodDatabase[0], NUM_FOODS);
        return (events ^ FOODLABEL_BUTTON_EVT);
    }

    if(events & FOODLABEL_WASTE_LED_EVT)
    {
        // Blink waste LED
        if(appState.coldChainBroken)
        {
            blinkWasteLED();

            // Restart timer for next blink (5 minutes)
            tmos_start_task(FoodLabel_TaskID, FOODLABEL_WASTE_LED_EVT, MS1_TO_SYSTEM_TIME(WASTE_LED_BLINK_PERIOD));
        }

        return (events ^ FOODLABEL_WASTE_LED_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      FoodLabel_ProcessTMOSMsg
 *
 * @brief   Process an incoming TMOS message
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void FoodLabel_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        case GAP_MSG_EVENT:
            // No specific GAP messages to handle for beacon mode
            break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      foodLabelStateNotificationCB
 *
 * @brief   Notification from the profile of a state change
 *
 * @param   newState - new state
 * @param   pEvent - event data
 *
 * @return  none
 */
static void foodLabelStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    switch(newState & GAPROLE_STATE_ADV_MASK)
    {
        case GAPROLE_STARTED:
            PRINT("BLE Initialized\n");
            break;

        case GAPROLE_ADVERTISING:
            PRINT("BLE Advertising\n");
            break;

        case GAPROLE_WAITING:
            PRINT("BLE Waiting\n");
            break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      performSensorRead
 *
 * @brief   Read temperature/humidity sensor and update state
 *
 * @return  none
 */
static void performSensorRead(void)
{
    float tempC, humidity;

    PRINT("Reading sensor...\n");

    // Read SHT4x sensor
    if(SHT4x_Read(&tempC, &humidity))
    {
        // Convert to Fahrenheit
        appState.currentTemp = (tempC * 9.0 / 5.0) + 32.0;
        appState.currentHumidity = humidity;

        PRINT("Temp: %.1f F (%.1f C), Humidity: %.1f%%\n",
              appState.currentTemp, tempC, appState.currentHumidity);

        // Check if temperature changed significantly
        float tempDiff = appState.currentTemp - appState.lastStoredTemp;
        if(tempDiff < 0) tempDiff = -tempDiff; // abs()

        PRINT("Temp diff: %.1f F\n", tempDiff);

        // Check cold chain integrity
        checkColdChain();

        // Update display if temp changed beyond threshold
        if(tempDiff >= TEMP_THRESHOLD)
        {
            PRINT("Temp threshold exceeded - updating display\n");
            updateDisplay(FALSE); // Partial refresh
            appState.lastStoredTemp = appState.currentTemp;

            // Save state to flash
            FlashStorage_Save(&appState);
        }

        // Update BLE advertisement with new data
        updateBLEAdvertisement();
    }
    else
    {
        PRINT("ERROR: Failed to read sensor\n");
    }
}

/*********************************************************************
 * @fn      checkColdChain
 *
 * @brief   Check if temperature is within safe range
 *
 * @return  none
 */
static void checkColdChain(void)
{
    const FoodItem_t *food = &foodDatabase[appState.currentFoodIndex];

    if(appState.currentTemp < food->minTempF || appState.currentTemp > food->maxTempF)
    {
        if(!appState.coldChainBroken)
        {
            appState.coldChainBroken = TRUE;
            PRINT("*** COLD CHAIN BROKEN ***\n");

            // Update status indicator on display
            EPaper_UpdateStatus(&appState);

            // Start waste LED blinking
            tmos_start_task(FoodLabel_TaskID, FOODLABEL_WASTE_LED_EVT, MS1_TO_SYSTEM_TIME(100));

            // Save state to flash
            FlashStorage_Save(&appState);
        }
    }
}

/*********************************************************************
 * @fn      calculateExpiryDate
 *
 * @brief   Calculate expiry date from created date + days left
 *
 * @return  none
 */
static void calculateExpiryDate(void)
{
    int day = appState.configCreatedDay;
    int month = appState.configCreatedMonth;
    int year = appState.configCreatedYear;
    int daysToAdd = appState.configDaysLeft;

    // Days in each month
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Add expiry days
    day += daysToAdd;

    // Handle month overflow
    while(day > daysInMonth[month - 1])
    {
        day -= daysInMonth[month - 1];
        month++;
        if(month > 12)
        {
            month = 1;
            year++;
        }
    }

    appState.configExpiryDay = day;
    appState.configExpiryMonth = month;
    appState.configExpiryYear = year;

    PRINT("Expiry: %02d-%02d-%02d (Days left: %d)\n",
          day, month, year, daysToAdd);
}

/*********************************************************************
 * @fn      updateDisplay
 *
 * @brief   Update e-paper display
 *
 * @param   fullRefresh - TRUE for full refresh, FALSE for partial
 *
 * @return  none
 */
static void updateDisplay(uint8_t fullRefresh)
{
    if(fullRefresh)
    {
        EPaper_ShowFullLabel(&appState, &foodDatabase[appState.currentFoodIndex]);
    }
    else
    {
        EPaper_UpdateTemperature(&appState);
    }
}

/*********************************************************************
 * @fn      blinkWasteLED
 *
 * @brief   Blink waste LED to indicate spoilage
 *
 * @return  none
 */
static void blinkWasteLED(void)
{
    PRINT("WASTE ALERT - Blinking LED\n");

    for(int i = 0; i < 10; i++)
    {
        GPIOA_SetBits(WASTE_LED_PIN);
        DelayMs(100);
        GPIOA_ResetBits(WASTE_LED_PIN);
        DelayMs(100);
    }
}

/*********************************************************************
 * @fn      buildAdvertData
 *
 * @brief   Build BLE advertisement data with temperature and status
 *
 * @return  none
 */
void buildAdvertData(void)
{
    uint8_t advIdx = 0;
    uint8_t scanIdx = 0;

    // === Advertisement Data (max 31 bytes) ===

    // Flags
    advertData[advIdx++] = 0x02; // Length
    advertData[advIdx++] = GAP_ADTYPE_FLAGS;
    advertData[advIdx++] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;

    // Manufacturer Specific Data (Company ID + Custom Data)
    advertData[advIdx++] = 0x0F; // Length (15 bytes: 2 company ID + 13 custom data)
    advertData[advIdx++] = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
    advertData[advIdx++] = LO_UINT16(WCH_COMPANY_ID);
    advertData[advIdx++] = HI_UINT16(WCH_COMPANY_ID);

    // Custom data format:
    // Byte 0: Food type index
    // Byte 1-2: Temperature (int16, in 0.1°F units)
    // Byte 3: Humidity (uint8, in %)
    // Byte 4: Days left
    // Byte 5: Flags (bit 0: cold chain broken, bit 1-7: reserved)
    advertData[advIdx++] = appState.currentFoodIndex;

    int16_t tempData = (int16_t)(appState.currentTemp * 10);
    advertData[advIdx++] = LO_UINT16(tempData);
    advertData[advIdx++] = HI_UINT16(tempData);

    advertData[advIdx++] = (uint8_t)appState.currentHumidity;
    advertData[advIdx++] = appState.configDaysLeft;

    uint8_t flags = 0;
    if(appState.coldChainBroken) flags |= 0x01;
    advertData[advIdx++] = flags;

    // Expiry date (3 bytes: DD-MM-YY)
    advertData[advIdx++] = appState.configExpiryDay;
    advertData[advIdx++] = appState.configExpiryMonth;
    advertData[advIdx++] = appState.configExpiryYear;

    // === Scan Response Data (max 31 bytes) ===

    // Complete local name
    const char *name = "SmartFood";
    uint8_t nameLen = strlen(name);

    scanRspData[scanIdx++] = nameLen + 1;
    scanRspData[scanIdx++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(&scanRspData[scanIdx], name, nameLen);
    scanIdx += nameLen;

    // TX Power Level
    scanRspData[scanIdx++] = 0x02;
    scanRspData[scanIdx++] = GAP_ADTYPE_POWER_LEVEL;
    scanRspData[scanIdx++] = 0; // 0 dBm

    PRINT("BLE advert data built (%d bytes)\n", advIdx);
}

/*********************************************************************
 * @fn      updateBLEAdvertisement
 *
 * @brief   Update BLE advertisement data with current sensor values
 *
 * @return  none
 */
static void updateBLEAdvertisement(void)
{
    // Rebuild advertisement data with current values
    buildAdvertData();

    // Update GAP parameters
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);

    PRINT("BLE advert updated\n");
}

/*********************************************************************
 * @fn      FoodLabel_TriggerButtonEvent
 *
 * @brief   Called by button interrupt to trigger button handling event
 *
 * @return  none
 */
void FoodLabel_TriggerButtonEvent(void)
{
    tmos_set_event(FoodLabel_TaskID, FOODLABEL_BUTTON_EVT);
}

/*********************************************************************
 * @fn      FoodLabel_SaveState
 *
 * @brief   Save application state to flash (called from button handler)
 *
 * @return  none
 */
void FoodLabel_SaveState(void)
{
    // Recalculate expiry date
    calculateExpiryDate();

    // Reset cold chain if product changed
    appState.coldChainBroken = FALSE;

    // Save to flash
    FlashStorage_Save(&appState);

    // Update display
    EPaper_ShowFullLabel(&appState, &foodDatabase[appState.currentFoodIndex]);

    // Update BLE advertisement
    updateBLEAdvertisement();

    PRINT("State saved\n");
}

/******************************** endfile @ foodlabel ******************************/
