/********************************** (C) COPYRIGHT *******************************
 * File Name          : epaper_gdey029t94.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-06
 * Description        : E-Paper driver for GDEY029T94 (2.9" 296x128, UC8151D controller)
 *******************************************************************************/

#include "epaper_driver.h"
#include "gfx.h"
#include "foodlabel.h"
#include "CH58x_common.h"
#include <string.h>
#include <stdio.h>

/*********************************************************************
 * CONSTANTS
 */

// UC8151D/IL0373 Commands for GDEY029T94
#define CMD_PANEL_SETTING                  0x00
#define CMD_POWER_SETTING                  0x01
#define CMD_POWER_OFF                      0x02
#define CMD_POWER_OFF_SEQUENCE             0x03
#define CMD_POWER_ON                       0x04
#define CMD_POWER_ON_MEASURE               0x05
#define CMD_BOOSTER_SOFT_START             0x06
#define CMD_DEEP_SLEEP                     0x07
#define CMD_DATA_START_TRANSMISSION_1      0x10
#define CMD_DATA_STOP                      0x11
#define CMD_DISPLAY_REFRESH                0x12
#define CMD_DATA_START_TRANSMISSION_2      0x13
#define CMD_VCOM_LUT                       0x20
#define CMD_W2W_LUT                        0x21
#define CMD_B2W_LUT                        0x22
#define CMD_W2B_LUT                        0x23
#define CMD_B2B_LUT                        0x24
#define CMD_PLL_CONTROL                    0x30
#define CMD_TEMPERATURE_SENSOR             0x40
#define CMD_TEMPERATURE_CALIBRATION        0x41
#define CMD_TEMPERATURE_SENSOR_WRITE       0x42
#define CMD_TEMPERATURE_SENSOR_READ        0x43
#define CMD_VCOM_AND_DATA_INTERVAL         0x50
#define CMD_LOW_POWER_DETECTION            0x51
#define CMD_TCON_SETTING                   0x60
#define CMD_RESOLUTION_SETTING             0x61
#define CMD_GET_STATUS                     0x71
#define CMD_AUTO_MEASURE_VCOM              0x80
#define CMD_READ_VCOM_VALUE                0x81
#define CMD_VCM_DC_SETTING                 0x82
#define CMD_PARTIAL_WINDOW                 0x90
#define CMD_PARTIAL_IN                     0x91
#define CMD_PARTIAL_OUT                    0x92
#define CMD_PROGRAM_MODE                   0xA0
#define CMD_ACTIVE_PROGRAM                 0xA1
#define CMD_READ_OTP                       0xA2
#define CMD_POWER_SAVING                   0xE3

// Display dimensions
#define EPD_WIDTH       296
#define EPD_HEIGHT      128

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t epaper_initialized = FALSE;
static uint8_t framebuffer[EPD_WIDTH * EPD_HEIGHT / 8];  // 1 bit per pixel = 4736 bytes

// LUT for fast partial refresh (GDEY029T94)
const unsigned char lut_partial[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// LUT for full refresh (GDEY029T94)
const unsigned char lut_full[] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22,
    0x22, 0x66, 0x69, 0x69, 0x59, 0x58, 0x99,
    0x99, 0x88, 0x00, 0x00, 0x00, 0x00, 0xF8,
    0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19,
    0x01, 0x00
};

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

    SPI0_MasterSendByte(cmd);

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

    SPI0_MasterSendByte(data);

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
        SPI0_MasterSendByte(data[i]);
    }

    GPIOA_SetBits(EPAPER_CS_PIN);     // CS HIGH
}

/*********************************************************************
 * @fn      EPaper_WaitUntilIdle
 *
 * @brief   Wait until BUSY pin goes LOW (display ready)
 *
 * @return  none
 */
static void EPaper_WaitUntilIdle(void)
{
    uint32_t timeout = 0;

    // BUSY is HIGH when display is busy, LOW when idle
    while(GPIOA_ReadPortPin(EPAPER_BUSY_PIN) == SET)
    {
        DelayMs(10);
        timeout++;

        if(timeout > 500)  // 5 second timeout
        {
            PRINT("EPaper timeout!\n");
            break;
        }
    }

    DelayMs(10);  // Additional delay for stability
}

/*********************************************************************
 * @fn      EPaper_HardwareReset
 *
 * @brief   Perform hardware reset sequence
 *
 * @return  none
 */
static void EPaper_HardwareReset(void)
{
    GPIOA_SetBits(EPAPER_RST_PIN);
    DelayMs(20);
    GPIOA_ResetBits(EPAPER_RST_PIN);
    DelayMs(10);
    GPIOA_SetBits(EPAPER_RST_PIN);
    DelayMs(20);
}

/*********************************************************************
 * @fn      EPaper_SetLUT_Full
 *
 * @brief   Load LUT for full refresh
 *
 * @return  none
 */
static void EPaper_SetLUT_Full(void)
{
    EPaper_SendCommand(CMD_VCOM_LUT);
    EPaper_SendDataBurst(lut_full, sizeof(lut_full));

    EPaper_WaitUntilIdle();
}

/*********************************************************************
 * @fn      EPaper_SetLUT_Partial
 *
 * @brief   Load LUT for partial refresh
 *
 * @return  none
 */
static void EPaper_SetLUT_Partial(void)
{
    EPaper_SendCommand(CMD_VCOM_LUT);
    EPaper_SendDataBurst(lut_partial, sizeof(lut_partial));

    EPaper_WaitUntilIdle();
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      EPaper_Init
 *
 * @brief   Initialize e-paper display (GDEY029T94 - UC8151D controller)
 *
 * @return  none
 */
void EPaper_Init(void)
{
    if(epaper_initialized)
    {
        return;
    }

    PRINT("EPaper Init (GDEY029T94 - UC8151D)\n");

    // Configure GPIO pins
    GPIOA_SetBits(EPAPER_CS_PIN | EPAPER_RST_PIN);
    GPIOA_ResetBits(EPAPER_DC_PIN);
    GPIOA_ModeCfg(EPAPER_CS_PIN | EPAPER_DC_PIN | EPAPER_RST_PIN, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(EPAPER_BUSY_PIN, GPIO_ModeIN_PU);

    // Configure SPI0 (PA12=SCK, PA13=MOSI)
    GPIOA_ModeCfg(GPIO_Pin_12 | GPIO_Pin_13, GPIO_ModeOut_PP_5mA);
    SPI0_MasterDefInit();

    // Hardware reset
    EPaper_HardwareReset();
    EPaper_WaitUntilIdle();

    // Power setting
    EPaper_SendCommand(CMD_POWER_SETTING);
    EPaper_SendData(0x03);  // VDS_EN, VDG_EN
    EPaper_SendData(0x00);  // VCOM_HV, VGHL_LV[1], VGHL_LV[0]
    EPaper_SendData(0x2B);  // VDH
    EPaper_SendData(0x2B);  // VDL
    EPaper_SendData(0x09);  // VDHR

    // Booster soft start
    EPaper_SendCommand(CMD_BOOSTER_SOFT_START);
    EPaper_SendData(0x07);
    EPaper_SendData(0x07);
    EPaper_SendData(0x17);

    // Power on
    EPaper_SendCommand(CMD_POWER_ON);
    EPaper_WaitUntilIdle();

    // Panel setting: KW-BF, UD, SHL, SHD_N, RST_N
    EPaper_SendCommand(CMD_PANEL_SETTING);
    EPaper_SendData(0xCF);  // LUT from OTP, B/W mode, scan up, shift right, booster ON

    // PLL control: 3A (100Hz), 3C (50Hz)
    EPaper_SendCommand(CMD_PLL_CONTROL);
    EPaper_SendData(0x3C);  // 50Hz

    // Resolution setting
    EPaper_SendCommand(CMD_RESOLUTION_SETTING);
    EPaper_SendData((EPD_WIDTH >> 8) & 0xFF);
    EPaper_SendData(EPD_WIDTH & 0xFF);
    EPaper_SendData((EPD_HEIGHT >> 8) & 0xFF);
    EPaper_SendData(EPD_HEIGHT & 0xFF);

    // VCOM and data interval setting
    EPaper_SendCommand(CMD_VCOM_AND_DATA_INTERVAL);
    EPaper_SendData(0x97);  // Border output: VCOM, data polarity: VCOM

    // Initialize graphics library
    GFX_Init(framebuffer, EPD_WIDTH, EPD_HEIGHT);

    epaper_initialized = TRUE;

    PRINT("EPaper initialized (296x128)\n");
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
    uint16_t width_bytes = EPD_WIDTH / 8;
    uint16_t height = EPD_HEIGHT;

    // Send white data to both buffers
    EPaper_SendCommand(CMD_DATA_START_TRANSMISSION_1);
    for(uint16_t i = 0; i < width_bytes * height; i++)
    {
        EPaper_SendData(0xFF);
    }

    EPaper_SendCommand(CMD_DATA_START_TRANSMISSION_2);
    for(uint16_t i = 0; i < width_bytes * height; i++)
    {
        EPaper_SendData(0xFF);
    }

    // Refresh display
    EPaper_SendCommand(CMD_DISPLAY_REFRESH);
    DelayMs(100);
    EPaper_WaitUntilIdle();

    // Clear framebuffer
    memset(framebuffer, 0xFF, sizeof(framebuffer));
}

/*********************************************************************
 * @fn      EPaper_Display
 *
 * @brief   Update entire display from framebuffer (full refresh)
 *
 * @return  none
 */
void EPaper_Display(void)
{
    uint16_t width_bytes = EPD_WIDTH / 8;
    uint16_t height = EPD_HEIGHT;

    // Send framebuffer to display
    EPaper_SendCommand(CMD_DATA_START_TRANSMISSION_2);

    for(uint16_t y = 0; y < height; y++)
    {
        for(uint16_t x = 0; x < width_bytes; x++)
        {
            uint16_t idx = x + y * width_bytes;
            EPaper_SendData(~framebuffer[idx]);  // Invert: 0=black, 1=white
        }
    }

    // Refresh display
    EPaper_SendCommand(CMD_DISPLAY_REFRESH);
    DelayMs(100);
    EPaper_WaitUntilIdle();
}

/*********************************************************************
 * @fn      EPaper_DisplayPartial
 *
 * @brief   Update display with partial refresh (faster, less flicker)
 *
 * @return  none
 */
void EPaper_DisplayPartial(void)
{
    // Use full refresh for now (partial refresh can cause ghosting)
    // For production, implement proper partial refresh with LUT
    EPaper_Display();
}

/*********************************************************************
 * @fn      EPaper_Sleep
 *
 * @brief   Put display into deep sleep mode
 *
 * @return  none
 */
void EPaper_Sleep(void)
{
    EPaper_SendCommand(CMD_VCOM_AND_DATA_INTERVAL);
    EPaper_SendData(0xF7);  // Border floating

    EPaper_SendCommand(CMD_POWER_OFF);
    EPaper_WaitUntilIdle();

    EPaper_SendCommand(CMD_DEEP_SLEEP);
    EPaper_SendData(0xA5);  // Check code
}

/*********************************************************************
 * @fn      EPaper_ShowSplash
 *
 * @brief   Display splash screen
 *
 * @return  none
 */
void EPaper_ShowSplash(void)
{
    GFX_ClearBuffer();

    // Draw "SMART LABELING" splash
    GFX_DrawString(40, 30, "SMART", GFX_FONT_LARGE, GFX_COLOR_BLACK);
    GFX_DrawString(20, 60, "LABELING", GFX_FONT_LARGE, GFX_COLOR_BLACK);
    GFX_DrawString(10, 100, "Cold Chain Monitoring", GFX_FONT_SMALL, GFX_COLOR_BLACK);

    EPaper_Display();
}

/*********************************************************************
 * @fn      EPaper_ShowFullLabel
 *
 * @brief   Display complete food label
 *
 * @param   state - application state
 * @param   food - food item data
 *
 * @return  none
 */
void EPaper_ShowFullLabel(const FoodLabelState_t *state, const FoodItem_t *food)
{
    char buf[32];

    GFX_ClearBuffer();

    // Product name (top left)
    GFX_DrawString(5, 10, (char*)food->type, GFX_FONT_LARGE, GFX_COLOR_BLACK);

    // Temperature box (top right)
    GFX_DrawRect(210, 5, 80, 55, GFX_COLOR_BLACK);
    GFX_DrawString(225, 15, "TEMP", GFX_FONT_SMALL, GFX_COLOR_BLACK);
    sprintf(buf, "%d", (int)state->currentTemp);
    GFX_DrawString(220, 35, buf, GFX_FONT_LARGE, GFX_COLOR_BLACK);
    GFX_DrawString(270, 35, "F", GFX_FONT_MEDIUM, GFX_COLOR_BLACK);

    // Created date
    sprintf(buf, "Created: %02d-%02d-%02d",
            state->configCreatedDay, state->configCreatedMonth, state->configCreatedYear);
    GFX_DrawString(5, 70, buf, GFX_FONT_SMALL, GFX_COLOR_BLACK);

    // Expiry date
    sprintf(buf, "Expires: %02d-%02d-%02d",
            state->configExpiryDay, state->configExpiryMonth, state->configExpiryYear);
    GFX_DrawString(5, 85, buf, GFX_FONT_SMALL, GFX_COLOR_BLACK);

    // Days left
    sprintf(buf, "Days left: %d", state->configDaysLeft);
    GFX_DrawString(5, 100, buf, GFX_FONT_SMALL, GFX_COLOR_BLACK);

    // Status
    if(state->coldChainBroken)
    {
        GFX_DrawString(5, 115, "Status: SPOILED!", GFX_FONT_SMALL, GFX_COLOR_BLACK);
        GFX_FillRect(205, 110, 85, 18, GFX_COLOR_BLACK);
        GFX_DrawString(215, 115, "SPOILED", GFX_FONT_SMALL, GFX_COLOR_WHITE);
    }
    else if(state->configDaysLeft <= 2)
    {
        GFX_DrawString(5, 115, "Status: Use Now", GFX_FONT_SMALL, GFX_COLOR_BLACK);
    }
    else if(state->configDaysLeft <= 5)
    {
        GFX_DrawString(5, 115, "Status: Use Soon", GFX_FONT_SMALL, GFX_COLOR_BLACK);
    }
    else
    {
        GFX_DrawString(5, 115, "Status: Fresh", GFX_FONT_SMALL, GFX_COLOR_BLACK);
    }

    // Logo (if not spoiled)
    if(!state->coldChainBroken)
    {
        GFX_FillRect(260, 110, 30, 16, GFX_COLOR_BLACK);
        GFX_DrawString(265, 115, "SL", GFX_FONT_SMALL, GFX_COLOR_WHITE);
    }

    EPaper_Display();
}

/*********************************************************************
 * @fn      EPaper_UpdateTemperature
 *
 * @brief   Update temperature value only (partial refresh)
 *
 * @param   state - application state
 *
 * @return  none
 */
void EPaper_UpdateTemperature(const FoodLabelState_t *state)
{
    // For GDEY029T94, use full refresh to avoid ghosting
    // In production, implement proper partial refresh
    char buf[16];

    // Update temperature in framebuffer
    GFX_FillRect(210, 5, 80, 55, GFX_COLOR_WHITE);
    GFX_DrawRect(210, 5, 80, 55, GFX_COLOR_BLACK);
    GFX_DrawString(225, 15, "TEMP", GFX_FONT_SMALL, GFX_COLOR_BLACK);
    sprintf(buf, "%d", (int)state->currentTemp);
    GFX_DrawString(220, 35, buf, GFX_FONT_LARGE, GFX_COLOR_BLACK);
    GFX_DrawString(270, 35, "F", GFX_FONT_MEDIUM, GFX_COLOR_BLACK);

    EPaper_DisplayPartial();
}

/*********************************************************************
 * @fn      EPaper_UpdateStatus
 *
 * @brief   Update cold chain status indicator
 *
 * @param   state - application state
 *
 * @return  none
 */
void EPaper_UpdateStatus(const FoodLabelState_t *state)
{
    // Clear status area
    GFX_FillRect(5, 110, 285, 18, GFX_COLOR_WHITE);

    if(state->coldChainBroken)
    {
        GFX_DrawString(5, 115, "Status: SPOILED!", GFX_FONT_SMALL, GFX_COLOR_BLACK);
        GFX_FillRect(205, 110, 85, 18, GFX_COLOR_BLACK);
        GFX_DrawString(215, 115, "SPOILED", GFX_FONT_SMALL, GFX_COLOR_WHITE);
    }
    else
    {
        GFX_DrawString(5, 115, "Status: Fresh", GFX_FONT_SMALL, GFX_COLOR_BLACK);
        GFX_FillRect(260, 110, 30, 16, GFX_COLOR_BLACK);
        GFX_DrawString(265, 115, "SL", GFX_FONT_SMALL, GFX_COLOR_WHITE);
    }

    EPaper_DisplayPartial();
}

/******************************** endfile @ epaper_gdey029t94 ******************************/
