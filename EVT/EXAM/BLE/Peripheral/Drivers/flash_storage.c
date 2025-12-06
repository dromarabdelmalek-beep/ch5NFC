/********************************** (C) COPYRIGHT *******************************
 * File Name          : flash_storage.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Flash storage driver for persistent application state
 *******************************************************************************/

#include "flash_storage.h"
#include "CH57x_common.h"

/*********************************************************************
 * @fn      FlashStorage_Init
 *
 * @brief   Initialize flash storage
 *
 * @return  none
 */
void FlashStorage_Init(void)
{
    PRINT("Flash storage init\n");
}

/*********************************************************************
 * @fn      FlashStorage_Save
 *
 * @brief   Save application state to flash
 *
 * @param   state - pointer to application state structure
 *
 * @return  none
 */
void FlashStorage_Save(FoodLabelState_t *state)
{
    uint8_t status;
    uint32_t flashAddr = FLASH_STORAGE_ADDR;

    PRINT("Saving state to flash...\n");

    // Erase flash block (4KB)
    status = FLASH_ROM_ERASE(flashAddr, 4096);
    if(status != SUCCESS)
    {
        PRINT("Flash erase failed: %d\n", status);
        return;
    }

    // Write state structure to flash
    status = FLASH_ROM_WRITE(flashAddr, (uint8_t *)state, sizeof(FoodLabelState_t));
    if(status != SUCCESS)
    {
        PRINT("Flash write failed: %d\n", status);
        return;
    }

    // Verify written data
    status = FLASH_ROM_VERIFY(flashAddr, (uint8_t *)state, sizeof(FoodLabelState_t));
    if(status != SUCCESS)
    {
        PRINT("Flash verify failed: %d\n", status);
        return;
    }

    PRINT("State saved successfully\n");
}

/*********************************************************************
 * @fn      FlashStorage_Load
 *
 * @brief   Load application state from flash
 *
 * @param   state - pointer to application state structure
 *
 * @return  none
 */
void FlashStorage_Load(FoodLabelState_t *state)
{
    uint32_t flashAddr = FLASH_STORAGE_ADDR;

    PRINT("Loading state from flash...\n");

    // Read state structure from flash
    FLASH_ROM_READ(flashAddr, (uint8_t *)state, sizeof(FoodLabelState_t));

    // Check if flash contains valid data (check boot count)
    if(state->bootCount == 0xFFFFFFFF || state->bootCount == 0)
    {
        PRINT("No valid data found, using defaults\n");

        // Initialize with default values
        state->currentFoodIndex = 0;
        state->lastStoredTemp = 77.0;
        state->coldChainBroken = FALSE;
        state->bootCount = 0;
        state->currentTemp = 77.0;
        state->currentHumidity = 0.0;
        state->configCreatedDay = 5;
        state->configCreatedMonth = 12;
        state->configCreatedYear = 25;
        state->configDaysLeft = 7;
        state->configExpiryDay = 12;
        state->configExpiryMonth = 12;
        state->configExpiryYear = 25;
        state->editMode = FALSE;
        state->selectedField = -1;
    }
    else
    {
        PRINT("State loaded successfully (boot count: %d)\n", (int)state->bootCount);
    }
}

/******************************** endfile @ flash_storage ******************************/
