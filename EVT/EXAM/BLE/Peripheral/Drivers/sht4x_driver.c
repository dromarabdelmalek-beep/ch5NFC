/********************************** (C) COPYRIGHT *******************************
 * File Name          : sht4x_driver.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : SHT4x temperature/humidity sensor driver using I2C
 *******************************************************************************/

#include "sht4x_driver.h"
#include "foodlabel.h"
#include "CH57x_common.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t i2c_initialized = FALSE;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SHT4x_WriteCommand
 *
 * @brief   Write a command to SHT4x sensor
 *
 * @param   cmd - command byte
 *
 * @return  TRUE on success, FALSE on failure
 */
static uint8_t SHT4x_WriteCommand(uint8_t cmd)
{
    uint8_t status;

    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(SHT4X_I2C_ADDR << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(cmd);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(ENABLE);

    return TRUE;
}

/*********************************************************************
 * @fn      SHT4x_ReadData
 *
 * @brief   Read data from SHT4x sensor
 *
 * @param   data - buffer to store read data
 * @param   len - number of bytes to read
 *
 * @return  TRUE on success, FALSE on failure
 */
static uint8_t SHT4x_ReadData(uint8_t *data, uint8_t len)
{
    uint8_t i;

    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(SHT4X_I2C_ADDR << 1, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(i = 0; i < len; i++)
    {
        if(i == len - 1)
        {
            // Last byte - send NACK
            I2C_AcknowledgeConfig(I2C_Ack_Disable);
        }
        else
        {
            I2C_AcknowledgeConfig(I2C_Ack_Enable);
        }

        while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED));
        data[i] = I2C_ReceiveData();
    }

    I2C_GenerateSTOP(ENABLE);
    I2C_AcknowledgeConfig(I2C_Ack_Enable);  // Re-enable ACK for next transaction

    return TRUE;
}

/*********************************************************************
 * @fn      SHT4x_CalculateCRC
 *
 * @brief   Calculate CRC-8 for SHT4x data verification
 *
 * @param   data - pointer to data
 * @param   len - data length
 *
 * @return  CRC-8 checksum
 */
static uint8_t SHT4x_CalculateCRC(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t i, j;

    for(i = 0; i < len; i++)
    {
        crc ^= data[i];
        for(j = 0; j < 8; j++)
        {
            if(crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SHT4x_Init
 *
 * @brief   Initialize SHT4x sensor and I2C peripheral
 *
 * @return  none
 */
void SHT4x_Init(void)
{
    if(i2c_initialized)
    {
        return;
    }

    PRINT("SHT4x Init\n");

    // Configure I2C pins
    GPIOA_ModeCfg(I2C_SDA_PIN | I2C_SCL_PIN, GPIO_ModeIN_PU);

    // Initialize I2C peripheral
    // 400kHz, 16:9 duty cycle, ACK enabled, 7-bit addressing
    I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9,
             I2C_Ack_Enable, I2C_AckAddr_7bit, 0x00);

    // Soft reset sensor
    SHT4x_WriteCommand(SHT4X_CMD_SOFT_RESET);

    // Wait for reset to complete (1ms)
    DelayMs(1);

    i2c_initialized = TRUE;

    PRINT("SHT4x initialized\n");
}

/*********************************************************************
 * @fn      SHT4x_Read
 *
 * @brief   Read temperature and humidity from SHT4x sensor
 *
 * @param   tempC - pointer to store temperature in Celsius
 * @param   humidity - pointer to store relative humidity in %
 *
 * @return  TRUE on success, FALSE on failure
 */
uint8_t SHT4x_Read(float *tempC, float *humidity)
{
    uint8_t data[6];
    uint16_t temp_raw, humid_raw;
    uint8_t temp_crc, humid_crc;

    if(!i2c_initialized)
    {
        PRINT("ERROR: SHT4x not initialized\n");
        return FALSE;
    }

    // Send measurement command (high precision)
    if(!SHT4x_WriteCommand(SHT4X_CMD_MEASURE_HIGH_PRECISION))
    {
        PRINT("ERROR: Failed to send measurement command\n");
        return FALSE;
    }

    // Wait for measurement to complete (10ms for high precision)
    DelayMs(10);

    // Read measurement data (6 bytes: temp_msb, temp_lsb, temp_crc, humid_msb, humid_lsb, humid_crc)
    if(!SHT4x_ReadData(data, 6))
    {
        PRINT("ERROR: Failed to read measurement data\n");
        return FALSE;
    }

    // Extract temperature data
    temp_raw = (data[0] << 8) | data[1];
    temp_crc = data[2];

    // Extract humidity data
    humid_raw = (data[3] << 8) | data[4];
    humid_crc = data[5];

    // Verify CRC for temperature
    if(SHT4x_CalculateCRC(&data[0], 2) != temp_crc)
    {
        PRINT("ERROR: Temperature CRC mismatch\n");
        return FALSE;
    }

    // Verify CRC for humidity
    if(SHT4x_CalculateCRC(&data[3], 2) != humid_crc)
    {
        PRINT("ERROR: Humidity CRC mismatch\n");
        return FALSE;
    }

    // Convert raw values to physical units
    // Temperature formula: T = -45 + 175 * (raw / 65535)
    *tempC = -45.0 + 175.0 * ((float)temp_raw / 65535.0);

    // Humidity formula: RH = -6 + 125 * (raw / 65535)
    *humidity = -6.0 + 125.0 * ((float)humid_raw / 65535.0);

    // Clamp humidity to 0-100%
    if(*humidity < 0.0) *humidity = 0.0;
    if(*humidity > 100.0) *humidity = 100.0;

    return TRUE;
}

/******************************** endfile @ sht4x_driver ******************************/
