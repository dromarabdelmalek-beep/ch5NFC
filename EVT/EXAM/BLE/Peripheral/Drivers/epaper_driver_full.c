/********************************** (C) COPYRIGHT *******************************
 * File Name          : epaper_driver_full.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Complete E-Paper display driver for 2.9" 296x128 (SSD1680)
 *******************************************************************************/

#include "epaper_driver.h"
#include "gfx.h"
#include "foodlabel.h"
#include "CH57x_common.h"
#include <string.h>
#include <stdio.h>

/*********************************************************************
 * CONSTANTS
 */

// SSD1680 Commands
#define CMD_DRIVER_OUTPUT_CONTROL          0x01
#define CMD_GATE_DRIVING_VOLTAGE           0x03
#define CMD_SOURCE_DRIVING_VOLTAGE         0x04
#define CMD_DEEP_SLEEP_MODE                0x10
#define CMD_DATA_ENTRY_MODE                0x11
#define CMD_SW_RESET                       0x12
#define CMD_TEMP_SENSOR_CONTROL            0x1A
#define CMD_MASTER_ACTIVATION              0x20
#define CMD_DISPLAY_UPDATE_CONTROL_1       0x21
#define CMD_DISPLAY_UPDATE_CONTROL_2       0x22
#define CMD_WRITE_RAM                      0x24
#define CMD_WRITE_VCOM_REGISTER            0x2C
#define CMD_WRITE_LUT_REGISTER             0x32
#define CMD_SET_DUMMY_LINE_PERIOD          0x3A
#define CMD_SET_GATE_TIME                  0x3B
#define CMD_BORDER_WAVEFORM_CONTROL        0x3C
#define CMD_SET_RAM_X_ADDRESS              0x44
#define CMD_SET_RAM_Y_ADDRESS              0x45
#define CMD_SET_RAM_X_COUNTER              0x4E
#define CMD_SET_RAM_Y_COUNTER              0x4F

// Display dimensions
#define EPD_WIDTH       296
#define EPD_HEIGHT      128

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t epaper_initialized = FALSE;
static uint8_t framebuffer[EPD_WIDTH * EPD_HEIGHT / 8];  // 1 bit per pixel

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      EPaper_SendCommand
 *
 * @brief   Send command to e-paper controller
 *
 * @param   cmd - command byte
 *
 * @return  none
 */
static void EPaper_SendCommand(uint8_t cmd)
{
    GPIOA_ResetBits(EPAPER_DC_PIN);  // DC LOW = Command
    GPIOA_ResetBits(EPAPER_CS_PIN);   // CS LOW

    SPI_MasterSendByte(cmd);

    GPIOA_SetBits(EPAPER_CS_PIN);     // CS HIGH
}

/*********************************************************************
 * @fn      EPaper_SendData
 *
 * @brief   Send data to e-paper controller
 *
 * @param   data - data byte
 *
 * @return  none
 */
static void EPaper_SendData(uint8_t data)
{
    GPIOA_SetBits(EPAPER_DC_PIN);     // DC HIGH = Data
    GPIOA_ResetBits(EPAPER_CS_PIN);   // CS LOW

    SPI_MasterSendByte(data);

    GPIOA_SetBits(EPAPER_CS_PIN);     // CS HIGH
}

/*********************************************************************
 * @fn      EPaper_SendDataBurst
 *
 * @brief   Send multiple data bytes
 *
 * @param   data - pointer to data
 * @param   len - number of bytes
 *
 * @return  none
 */
static void EPaper_SendDataBurst(const uint8_t *data, uint16_t len)
{
    GPIOA_SetBits(EPAPER_DC_PIN);     // DC HIGH = Data
    GPIOA_ResetBits(EPAPER_CS_PIN);   // CS LOW

    for(uint16_t i = 0; i < len; i++)
    {
        SPI_MasterSendByte(data[i]);
    }

    GPIOA_SetBits(EPAPER_CS_PIN);     // CS HIGH
}

/*********************************************************************
 * @fn      EPaper_WaitUntilIdle
 *
 * @brief   Wait until e-paper display is idle
 *
 * @return  none
 */
static void EPaper_WaitUntilIdle(void)
{
    uint32_t timeout = 0;

    // Wait while BUSY is HIGH (display is busy)
    while(GPIOA_ReadPortPin(EPAPER_BUSY_PIN) && timeout < 5000)
    {
        DelayMs(10);
        timeout += 10;
    }

    if(timeout >= 5000)
    {
        PRINT("EPaper: Timeout waiting for idle\n");
    }
}

/*********************************************************************
 * @fn      EPaper_HardwareReset
 *
 * @brief   Perform hardware reset
 *
 * @return  none
 */
static void EPaper_HardwareReset(void)
{
    GPIOA_SetBits(EPAPER_RST_PIN);
    DelayMs(20);
    GPIOA_ResetBits(EPAPER_RST_PIN);
    DelayMs(2);
    GPIOA_SetBits(EPAPER_RST_PIN);
    DelayMs(20);
}

/*********************************************************************
 * @fn      EPaper_SetWindow
 *
 * @brief   Set display window
 *
 * @param   x_start - start X coordinate
 * @param   y_start - start Y coordinate
 * @param   x_end - end X coordinate
 * @param   y_end - end Y coordinate
 *
 * @return  none
 */
static void EPaper_SetWindow(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    // Set RAM X address
    EPaper_SendCommand(CMD_SET_RAM_X_ADDRESS);
    EPaper_SendData(x_start / 8);
    EPaper_SendData(x_end / 8);

    // Set RAM Y address
    EPaper_SendCommand(CMD_SET_RAM_Y_ADDRESS);
    EPaper_SendData(y_start & 0xFF);
    EPaper_SendData((y_start >> 8) & 0xFF);
    EPaper_SendData(y_end & 0xFF);
    EPaper_SendData((y_end >> 8) & 0xFF);
}

/*********************************************************************
 * @fn      EPaper_SetCursor
 *
 * @brief   Set cursor position
 *
 * @param   x - X coordinate
 * @param   y - Y coordinate
 *
 * @return  none
 */
static void EPaper_SetCursor(uint16_t x, uint16_t y)
{
    EPaper_SendCommand(CMD_SET_RAM_X_COUNTER);
    EPaper_SendData(x / 8);

    EPaper_SendCommand(CMD_SET_RAM_Y_COUNTER);
    EPaper_SendData(y & 0xFF);
    EPaper_SendData((y >> 8) & 0xFF);
}

/*********************************************************************
 * @fn      EPaper_TurnOnDisplay
 *
 * @brief   Trigger display update
 *
 * @return  none
 */
static void EPaper_TurnOnDisplay(void)
{
    EPaper_SendCommand(CMD_DISPLAY_UPDATE_CONTROL_2);
    EPaper_SendData(0xF7);  // Enable clock, enable analog, load temperature, display pattern, disable analog, disable clock

    EPaper_SendCommand(CMD_MASTER_ACTIVATION);

    EPaper_WaitUntilIdle();
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      EPaper_Init
 *
 * @brief   Initialize e-paper display (SSD1680 controller)
 *
 * @return  none
 */
void EPaper_Init(void)
{
    if(epaper_initialized)
    {
        return;
    }

    PRINT("EPaper Init (SSD1680)\n");

    // Configure GPIO pins
    GPIOA_SetBits(EPAPER_CS_PIN | EPAPER_RST_PIN);
    GPIOA_ResetBits(EPAPER_DC_PIN);
    GPIOA_ModeCfg(EPAPER_CS_PIN | EPAPER_DC_PIN | EPAPER_RST_PIN, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(EPAPER_BUSY_PIN, GPIO_ModeIN_PU);

    // Configure SPI
    GPIOA_ModeCfg(GPIO_Pin_5 | GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
    SPI_MasterDefInit();

    // Hardware reset
    EPaper_HardwareReset();

    EPaper_WaitUntilIdle();

    // Software reset
    EPaper_SendCommand(CMD_SW_RESET);
    EPaper_WaitUntilIdle();

    // Driver output control (296x128)
    EPaper_SendCommand(CMD_DRIVER_OUTPUT_CONTROL);
    EPaper_SendData((EPD_HEIGHT - 1) & 0xFF);
    EPaper_SendData(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    EPaper_SendData(0x00);  // GD = 0, SM = 0, TB = 0

    // Data entry mode
    EPaper_SendCommand(CMD_DATA_ENTRY_MODE);
    EPaper_SendData(0x03);  // X increment, Y increment

    // Set window
    EPaper_SetWindow(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);

    // Border waveform
    EPaper_SendCommand(CMD_BORDER_WAVEFORM_CONTROL);
    EPaper_SendData(0x05);

    // Temperature sensor control
    EPaper_SendCommand(CMD_TEMP_SENSOR_CONTROL);
    EPaper_SendData(0x80);  // Internal temperature sensor

    // Display update control
    EPaper_SendCommand(CMD_DISPLAY_UPDATE_CONTROL_1);
    EPaper_SendData(0x00);
    EPaper_SendData(0x80);

    // Set cursor to origin
    EPaper_SetCursor(0, 0);

    EPaper_WaitUntilIdle();

    // Initialize graphics library
    GFX_Init(framebuffer, EPD_WIDTH, EPD_HEIGHT);

    epaper_initialized = TRUE;

    PRINT("EPaper initialized\n");
}

/*********************************************************************
 * @fn      EPaper_Clear
 *
 * @brief   Clear display (fill with white)
 *
 * @return  none
 */
void EPaper_Clear(void)
{
    EPaper_SetWindow(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
    EPaper_SetCursor(0, 0);

    EPaper_SendCommand(CMD_WRITE_RAM);

    for(uint16_t i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++)
    {
        EPaper_SendData(0xFF);  // White
    }

    EPaper_TurnOnDisplay();
}

/*********************************************************************
 * @fn      EPaper_UpdateFull
 *
 * @brief   Update full display from framebuffer
 *
 * @return  none
 */
static void EPaper_UpdateFull(void)
{
    EPaper_SetWindow(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
    EPaper_SetCursor(0, 0);

    EPaper_SendCommand(CMD_WRITE_RAM);
    EPaper_SendDataBurst(framebuffer, sizeof(framebuffer));

    EPaper_TurnOnDisplay();
}

/*********************************************************************
 * @fn      EPaper_ShowSplash
 *
 * @brief   Show splash screen with "SmartFood" logo
 *
 * @return  none
 */
void EPaper_ShowSplash(void)
{
    PRINT("EPaper ShowSplash\n");

    GFX_Clear();

    // Draw title
    GFX_DrawString(80, 40, "SmartFood", &Font16x24, 0);
    GFX_DrawString(60, 70, "Cold Chain Monitor", &Font8x8, 0);

    EPaper_UpdateFull();

    PRINT("Splash screen displayed\n");
}

/*********************************************************************
 * @fn      EPaper_ShowFullLabel
 *
 * @brief   Show full food label with all information
 *
 * @param   state - application state
 * @param   food - food item information
 *
 * @return  none
 */
void EPaper_ShowFullLabel(FoodLabelState_t *state, const FoodItem_t *food)
{
    char buffer[32];

    PRINT("EPaper ShowFullLabel: %s\n", food->type);

    GFX_Clear();

    // Product name (top left)
    GFX_DrawString(5, 10, food->type, &Font16x24, 0);

    // Temperature box (top right) - filled rounded rect
    GFX_FillRoundRect(210, 2, 82, 58, 8, 0);

    // Draw "TEMP" label (white text on black background)
    GFX_DrawString(230, 12, "TEMP", &Font8x8, 1);
    GFX_DrawLine(218, 22, 284, 22, 1);

    // Draw temperature value
    sprintf(buffer, "%d", (int)state->currentTemp);
    GFX_DrawString(220, 35, buffer, &Font16x24, 1);
    GFX_DrawString(260, 35, "o", &Font8x8, 1);
    GFX_DrawString(268, 35, "F", &Font16x24, 1);

    // Created date
    sprintf(buffer, "Created: %02d-%02d-%02d",
            state->configCreatedDay, state->configCreatedMonth, state->configCreatedYear);
    GFX_DrawString(5, 70, buffer, &Font8x8, 0);

    // Expiry date
    sprintf(buffer, "Expires: %02d-%02d-%02d",
            state->configExpiryDay, state->configExpiryMonth, state->configExpiryYear);
    GFX_DrawString(5, 85, buffer, &Font8x8, 0);

    // Days left
    sprintf(buffer, "Days left: %d", state->configDaysLeft);
    GFX_DrawString(5, 100, buffer, &Font8x8, 0);

    // Status
    const char *status;
    if(state->coldChainBroken)
    {
        status = "Status: SPOILED!";
    }
    else if(state->configDaysLeft <= 2)
    {
        status = "Status: Use Now";
    }
    else if(state->configDaysLeft <= 5)
    {
        status = "Status: Use Soon";
    }
    else
    {
        status = "Status: Fresh";
    }
    GFX_DrawString(5, 115, status, &Font8x8, 0);

    // Status indicator (bottom right)
    if(state->coldChainBroken)
    {
        GFX_FillRoundRect(205, 110, 85, 18, 3, 0);
        GFX_DrawString(215, 116, "SPOILED", &Font8x8, 1);
    }
    else
    {
        GFX_FillRoundRect(256, 112, 34, 16, 3, 0);
        GFX_DrawString(264, 118, "SL", &Font8x8, 1);
    }

    EPaper_UpdateFull();

    PRINT("Full label displayed\n");
}

/*********************************************************************
 * @fn      EPaper_UpdateTemperature
 *
 * @brief   Update temperature display (partial refresh would go here)
 *
 * @param   state - application state
 *
 * @return  none
 */
void EPaper_UpdateTemperature(FoodLabelState_t *state)
{
    char buffer[16];

    PRINT("EPaper UpdateTemperature: %.1f F\n", state->currentTemp);

    // For simplicity, redraw temperature box
    // In production, use partial refresh
    GFX_FillRect(220, 35, 70, 24, 1);  // Clear temp area

    sprintf(buffer, "%d", (int)state->currentTemp);
    GFX_DrawString(220, 35, buffer, &Font16x24, 1);
    GFX_DrawString(260, 35, "o", &Font8x8, 1);
    GFX_DrawString(268, 35, "F", &Font16x24, 1);

    EPaper_UpdateFull();

    PRINT("Temperature updated\n");
}

/*********************************************************************
 * @fn      EPaper_UpdateStatus
 *
 * @brief   Update status indicator
 *
 * @param   state - application state
 *
 * @return  none
 */
void EPaper_UpdateStatus(FoodLabelState_t *state)
{
    PRINT("EPaper UpdateStatus: %s\n", state->coldChainBroken ? "SPOILED" : "OK");

    // Clear status area
    GFX_FillRect(205, 110, 90, 18, 1);

    // Redraw status
    if(state->coldChainBroken)
    {
        GFX_FillRoundRect(205, 110, 85, 18, 3, 0);
        GFX_DrawString(215, 116, "SPOILED", &Font8x8, 1);
    }
    else
    {
        GFX_FillRoundRect(256, 112, 34, 16, 3, 0);
        GFX_DrawString(264, 118, "SL", &Font8x8, 1);
    }

    EPaper_UpdateFull();

    PRINT("Status updated\n");
}

/*********************************************************************
 * @fn      EPaper_UpdateProductHighlight
 *
 * @brief   Update product name with highlight (for edit mode)
 *
 * @param   state - application state
 * @param   food - food item information
 * @param   highlighted - TRUE to highlight, FALSE to clear
 *
 * @return  none
 */
void EPaper_UpdateProductHighlight(FoodLabelState_t *state, const FoodItem_t *food, uint8_t highlighted)
{
    PRINT("EPaper UpdateProductHighlight: %s (highlighted: %d)\n", food->type, highlighted);

    // Clear product area
    GFX_FillRect(5, 10, 200, 30, 1);

    if(highlighted)
    {
        // Draw black background
        GFX_FillRect(5, 10, 195, 24, 0);
        // Draw white text
        GFX_DrawString(7, 12, food->type, &Font16x24, 1);
    }
    else
    {
        // Draw normal (black text)
        GFX_DrawString(5, 10, food->type, &Font16x24, 0);
    }

    EPaper_UpdateFull();

    PRINT("Product highlight updated\n");
}

/*********************************************************************
 * @fn      EPaper_UpdateCreatedDateHighlight
 *
 * @brief   Update created date with field highlight (for edit mode)
 *
 * @param   state - application state
 * @param   highlighted - field to highlight (1=day, 2=month, 3=year)
 *
 * @return  none
 */
void EPaper_UpdateCreatedDateHighlight(FoodLabelState_t *state, uint8_t highlighted)
{
    char buffer[32];

    PRINT("EPaper UpdateCreatedDateHighlight (field: %d)\n", highlighted);

    // Clear date area
    GFX_FillRect(5, 70, 150, 10, 1);

    // Draw "Created: "
    GFX_DrawString(5, 70, "Created: ", &Font8x8, 0);

    uint16_t x = 75;

    // Draw day
    sprintf(buffer, "%02d", state->configCreatedDay);
    if(state->selectedField == 1)
    {
        GFX_FillRect(x - 2, 67, 18, 12, 0);
        GFX_DrawString(x, 70, buffer, &Font8x8, 1);
    }
    else
    {
        GFX_DrawString(x, 70, buffer, &Font8x8, 0);
    }
    x += 18;

    GFX_DrawString(x, 70, "-", &Font8x8, 0);
    x += 8;

    // Draw month
    sprintf(buffer, "%02d", state->configCreatedMonth);
    if(state->selectedField == 2)
    {
        GFX_FillRect(x - 2, 67, 18, 12, 0);
        GFX_DrawString(x, 70, buffer, &Font8x8, 1);
    }
    else
    {
        GFX_DrawString(x, 70, buffer, &Font8x8, 0);
    }
    x += 18;

    GFX_DrawString(x, 70, "-", &Font8x8, 0);
    x += 8;

    // Draw year
    sprintf(buffer, "%02d", state->configCreatedYear);
    if(state->selectedField == 3)
    {
        GFX_FillRect(x - 2, 67, 18, 12, 0);
        GFX_DrawString(x, 70, buffer, &Font8x8, 1);
    }
    else
    {
        GFX_DrawString(x, 70, buffer, &Font8x8, 0);
    }

    EPaper_UpdateFull();

    PRINT("Created date highlight updated\n");
}

/*********************************************************************
 * @fn      EPaper_UpdateDaysLeftHighlight
 *
 * @brief   Update days left with highlight (for edit mode)
 *
 * @param   state - application state
 * @param   highlighted - TRUE to highlight, FALSE to clear
 *
 * @return  none
 */
void EPaper_UpdateDaysLeftHighlight(FoodLabelState_t *state, uint8_t highlighted)
{
    char buffer[32];

    PRINT("EPaper UpdateDaysLeftHighlight: %d days (highlighted: %d)\n",
          state->configDaysLeft, highlighted);

    // Clear days left area
    GFX_FillRect(5, 100, 150, 10, 1);

    // Draw "Days left: "
    GFX_DrawString(5, 100, "Days left: ", &Font8x8, 0);

    uint16_t x = 90;

    sprintf(buffer, "%d", state->configDaysLeft);
    if(highlighted)
    {
        uint16_t width = (state->configDaysLeft >= 10) ? 18 : 10;
        GFX_FillRect(x - 2, 97, width, 12, 0);
        GFX_DrawString(x, 100, buffer, &Font8x8, 1);
    }
    else
    {
        GFX_DrawString(x, 100, buffer, &Font8x8, 0);
    }

    // Also update expiry date
    GFX_FillRect(5, 85, 150, 10, 1);
    sprintf(buffer, "Expires: %02d-%02d-%02d",
            state->configExpiryDay, state->configExpiryMonth, state->configExpiryYear);
    GFX_DrawString(5, 85, buffer, &Font8x8, 0);

    EPaper_UpdateFull();

    PRINT("Days left highlight updated\n");
}

/******************************** endfile @ epaper_driver_full ******************************/
