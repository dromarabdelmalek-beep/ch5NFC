/********************************** (C) COPYRIGHT *******************************
 * File Name          : button_driver.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Button handling driver with GPIO interrupts and debouncing
 *******************************************************************************/

#include "button_driver.h"
#include "foodlabel.h"
#include "CH57x_common.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint32_t lastButtonPressTime = 0;
static uint32_t buttonPressStartTime = 0;
static uint8_t btn1WasPressed = FALSE;

// External reference to application state and food database
extern FoodLabelState_t appState;
extern const FoodItem_t foodDatabase[];
extern const int NUM_FOODS;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint32_t GetCurrentTimeMs(void);
static void calculateExpiryDateLocal(FoodLabelState_t *state);

/*********************************************************************
 * @fn      GetCurrentTimeMs
 *
 * @brief   Get current time in milliseconds using RTC
 *
 * @return  Current time in ms
 */
static uint32_t GetCurrentTimeMs(void)
{
    // Use RTC cycle counter (LSI runs at ~32kHz)
    // Approximate conversion to ms
    return RTC_GetCycleLSI() / 32;  // Rough approximation: 32768 / 32 ≈ 1024 Hz ≈ 1 ms
}

/*********************************************************************
 * @fn      calculateExpiryDateLocal
 *
 * @brief   Calculate expiry date from created date + days left
 *
 * @param   state - pointer to application state
 *
 * @return  none
 */
static void calculateExpiryDateLocal(FoodLabelState_t *state)
{
    int day = state->configCreatedDay;
    int month = state->configCreatedMonth;
    int year = state->configCreatedYear;
    int daysToAdd = state->configDaysLeft;

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

    state->configExpiryDay = day;
    state->configExpiryMonth = month;
    state->configExpiryYear = year;
}

/*********************************************************************
 * @fn      GPIOA_IRQHandler
 *
 * @brief   GPIO interrupt handler for buttons
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void GPIOA_IRQHandler(void)
{
    // Check which button triggered the interrupt
    if(GPIOA_ReadITFlagBit(BTN_FIELD))
    {
        GPIOA_ClearITFlagBit(BTN_FIELD);

        // Trigger button event in main task
        FoodLabel_TriggerButtonEvent();
    }

    if(GPIOA_ReadITFlagBit(BTN_VALUE_UP))
    {
        GPIOA_ClearITFlagBit(BTN_VALUE_UP);

        // Trigger button event in main task
        FoodLabel_TriggerButtonEvent();
    }

    if(GPIOA_ReadITFlagBit(BTN_VALUE_DOWN))
    {
        GPIOA_ClearITFlagBit(BTN_VALUE_DOWN);

        // Trigger button event in main task
        FoodLabel_TriggerButtonEvent();
    }
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Buttons_Init
 *
 * @brief   Initialize button GPIOs with interrupts
 *
 * @return  none
 */
void Buttons_Init(void)
{
    PRINT("Buttons Init\n");

    // Configure button pins as input with pull-down
    // (Buttons connect to 3.3V when pressed)
    GPIOA_ModeCfg(BTN_FIELD | BTN_VALUE_UP | BTN_VALUE_DOWN, GPIO_ModeIN_PD);

    // Configure interrupts for rising edge (button press)
    GPIOA_ITModeCfg(BTN_FIELD, GPIO_ITMode_RiseEdge);
    GPIOA_ITModeCfg(BTN_VALUE_UP, GPIO_ITMode_RiseEdge);
    GPIOA_ITModeCfg(BTN_VALUE_DOWN, GPIO_ITMode_RiseEdge);

    // Enable GPIO interrupt
    PFIC_EnableIRQ(GPIO_A_IRQn);

    // Initialize LED pins
    GPIOA_SetBits(LED_PIN);
    GPIOA_ResetBits(WASTE_LED_PIN);
    GPIOA_ModeCfg(LED_PIN | WASTE_LED_PIN, GPIO_ModeOut_PP_5mA);

    PRINT("Buttons initialized\n");
}

/*********************************************************************
 * @fn      Buttons_Handle
 *
 * @brief   Handle button press events (called from TMOS task)
 *
 * @param   state - pointer to application state
 * @param   foodDatabase - pointer to food database array
 * @param   numFoods - number of foods in database
 *
 * @return  none
 */
void Buttons_Handle(FoodLabelState_t *state, const FoodItem_t *foodDatabase, int numFoods)
{
    uint32_t currentTime = GetCurrentTimeMs();

    // Debounce check
    if((currentTime - lastButtonPressTime) < BUTTON_DEBOUNCE_MS)
    {
        return;
    }

    // Read current button states
    uint8_t btn1 = GPIOA_ReadPortPin(BTN_FIELD);
    uint8_t btn2 = GPIOA_ReadPortPin(BTN_VALUE_UP);
    uint8_t btn3 = GPIOA_ReadPortPin(BTN_VALUE_DOWN);

    // Long press detection on Button 1 (Field Select)
    if(btn1 && !btn1WasPressed)
    {
        buttonPressStartTime = currentTime;
        btn1WasPressed = TRUE;
    }

    if(btn1WasPressed && btn1)
    {
        uint32_t pressDuration = currentTime - buttonPressStartTime;

        if(pressDuration >= BUTTON_LONG_PRESS_MS)
        {
            lastButtonPressTime = currentTime;
            btn1WasPressed = FALSE;

            // Toggle edit mode
            state->editMode = !state->editMode;

            if(state->editMode)
            {
                PRINT("*** EDIT MODE ***\n");
                state->selectedField = 0;

                // Blink LED to indicate edit mode entered
                for(int i = 0; i < 5; i++)
                {
                    GPIOA_SetBits(LED_PIN);
                    DelayMs(100);
                    GPIOA_ResetBits(LED_PIN);
                    DelayMs(100);
                }
            }
            else
            {
                PRINT("*** VIEW MODE ***\n");
                state->selectedField = -1;

                // Save changes and update display
                FoodLabel_SaveState();

                // Blink LED to indicate edit mode exited
                for(int i = 0; i < 3; i++)
                {
                    GPIOA_SetBits(LED_PIN);
                    DelayMs(200);
                    GPIOA_ResetBits(LED_PIN);
                    DelayMs(200);
                }
            }

            return;
        }
    }

    // Short press on Button 1 - Switch Field (only in edit mode)
    if(!btn1 && btn1WasPressed)
    {
        uint32_t pressDuration = currentTime - buttonPressStartTime;
        btn1WasPressed = FALSE;

        if(pressDuration < BUTTON_LONG_PRESS_MS && state->editMode)
        {
            lastButtonPressTime = currentTime;

            // Cycle through fields: 0=Product, 1=Day, 2=Month, 3=Year, 4=DaysLeft
            state->selectedField = (state->selectedField + 1) % 5;

            const char* fields[] = {"Product", "Created Day", "Created Month", "Created Year", "Days Left"};
            PRINT("Field: %s\n", fields[state->selectedField]);

            // TODO: Update display to show field highlight
        }
    }

    // Button 2 - Increase Value (only in edit mode)
    if(btn2 && state->editMode && state->selectedField >= 0)
    {
        lastButtonPressTime = currentTime;

        switch(state->selectedField)
        {
            case 0: // Product
                state->currentFoodIndex = (state->currentFoodIndex + 1) % numFoods;
                state->configDaysLeft = foodDatabase[state->currentFoodIndex].daysToExpiry;
                PRINT("Product: %s\n", foodDatabase[state->currentFoodIndex].type);
                break;

            case 1: // Created Day
                state->configCreatedDay++;
                if(state->configCreatedDay > 31) state->configCreatedDay = 1;
                PRINT("Created Day: %d\n", state->configCreatedDay);
                break;

            case 2: // Created Month
                state->configCreatedMonth++;
                if(state->configCreatedMonth > 12) state->configCreatedMonth = 1;
                PRINT("Created Month: %d\n", state->configCreatedMonth);
                break;

            case 3: // Created Year
                state->configCreatedYear++;
                if(state->configCreatedYear > 30) state->configCreatedYear = 25;
                PRINT("Created Year: %d\n", state->configCreatedYear);
                break;

            case 4: // Days Left
                state->configDaysLeft++;
                if(state->configDaysLeft > 90) state->configDaysLeft = 1;
                PRINT("Days Left: %d\n", state->configDaysLeft);
                calculateExpiryDateLocal(state);
                break;
        }

        // TODO: Update display partial refresh
    }

    // Button 3 - Decrease Value (only in edit mode)
    if(btn3 && state->editMode && state->selectedField >= 0)
    {
        lastButtonPressTime = currentTime;

        switch(state->selectedField)
        {
            case 0: // Product
                state->currentFoodIndex--;
                if(state->currentFoodIndex < 0) state->currentFoodIndex = numFoods - 1;
                state->configDaysLeft = foodDatabase[state->currentFoodIndex].daysToExpiry;
                PRINT("Product: %s\n", foodDatabase[state->currentFoodIndex].type);
                break;

            case 1: // Created Day
                state->configCreatedDay--;
                if(state->configCreatedDay < 1) state->configCreatedDay = 31;
                PRINT("Created Day: %d\n", state->configCreatedDay);
                break;

            case 2: // Created Month
                state->configCreatedMonth--;
                if(state->configCreatedMonth < 1) state->configCreatedMonth = 12;
                PRINT("Created Month: %d\n", state->configCreatedMonth);
                break;

            case 3: // Created Year
                state->configCreatedYear--;
                if(state->configCreatedYear < 25) state->configCreatedYear = 30;
                PRINT("Created Year: %d\n", state->configCreatedYear);
                break;

            case 4: // Days Left
                state->configDaysLeft--;
                if(state->configDaysLeft < 1) state->configDaysLeft = 90;
                PRINT("Days Left: %d\n", state->configDaysLeft);
                calculateExpiryDateLocal(state);
                break;
        }

        // TODO: Update display partial refresh
    }
}

/******************************** endfile @ button_driver ******************************/
