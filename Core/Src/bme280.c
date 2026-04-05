#include "bme280.h"

/* Registers */
#define REG_ID          0xD0
#define REG_RESET       0xE0
#define REG_CTRL_HUM    0xF2
#define REG_STATUS      0xF3
#define REG_CTRL_MEAS   0xF4
#define REG_CONFIG      0xF5
#define REG_PRESS_MSB   0xF7  // ... to 0xFE


/* >>>wr8: ==============================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Writes an 8-bit value to a specified register of the BME280 sensor over I2C.
 * Inputs: *dev - Pointer to BME280_Handle structure containing I2C handle and address, reg - Register address, data - 8-bit data to write, data - 8-bit data to write
 * Returns: HAL_Mem_Write status
 */
static HAL_StatusTypeDef wr8(BME280_Handle *dev, uint8_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}// eo wr8::

/*>>rd:==================================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Reads multiple bytes from a specified register of the BME280 sensor over I2C.
 * Inputs: *dev - Pointer to BME280_Handle structure containing I2C handle and address, reg - Register address, *buf - Buffer to store read data, len - Number of bytes to read
 * Returns: HAL_Mem_Read status
 */
static HAL_StatusTypeDef rd(BME280_Handle *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}// eo rd::

/* u16_le:=============================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Converts two bytes in little-endian format to a 16-bit unsigned integer.
 * Inputs: *p - Pointer to the byte array
 * Returns: 16-bit unsigned integer
 */
static uint16_t u16_le(const uint8_t *p){ return (uint16_t)p[0] | ((uint16_t)p[1] << 8); } // eo u16_le::

/* s16_le:=============================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Converts two bytes in little-endian format to a 16-bit signed integer.
 * Inputs: *p - Pointer to the byte array
 * Returns: 16-bit signed integer
 */
static int16_t  s16_le(const uint8_t *p){ return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); } // eo s16_le::


/* bme280_init:==========================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Initializes the BME280 sensor, reads its ID and calibration data, and configures it for measurement.
 * Inputs: *dev - Pointer to BME280_Handle structure, *hi2c - Pointer to I2C handle, i2c_addr - I2C address of the BME280 sensor
 * Returns: BME280_Status indicating success or type of error
 */
BME280_Status bme280_init(BME280_Handle *dev, I2C_HandleTypeDef *hi2c, uint8_t i2c_addr)
{
    dev->hi2c = hi2c;
    dev->i2c_addr = i2c_addr;

    uint8_t id = 0;
    if (rd(dev, REG_ID, &id, 1) != HAL_OK) return BME280_ERR;
    if (id != 0x60) return BME280_ERR_ID; // BME280 ID

    // Soft reset
    (void)wr8(dev, REG_RESET, 0xB6);
    HAL_Delay(3);

    // Read calibration
    uint8_t calib1[26]; // 0x88..0xA1
    if (rd(dev, 0x88, calib1, sizeof(calib1)) != HAL_OK) return BME280_ERR;

    dev->dig_T1 = u16_le(&calib1[0]);
    dev->dig_T2 = s16_le(&calib1[2]);
    dev->dig_T3 = s16_le(&calib1[4]);
    dev->dig_P1 = u16_le(&calib1[6]);
    dev->dig_P2 = s16_le(&calib1[8]);
    dev->dig_P3 = s16_le(&calib1[10]);
    dev->dig_P4 = s16_le(&calib1[12]);
    dev->dig_P5 = s16_le(&calib1[14]);
    dev->dig_P6 = s16_le(&calib1[16]);
    dev->dig_P7 = s16_le(&calib1[18]);
    dev->dig_P8 = s16_le(&calib1[20]);
    dev->dig_P9 = s16_le(&calib1[22]);
    dev->dig_H1 = calib1[24];

    uint8_t calib2[7]; // 0xE1..0xE7
    if (rd(dev, 0xE1, calib2, sizeof(calib2)) != HAL_OK) return BME280_ERR;
    dev->dig_H2 = s16_le(&calib2[0]);
    dev->dig_H3 = calib2[2];
    dev->dig_H4 = (int16_t)((((int16_t)calib2[3]) << 4) | (calib2[4] & 0x0F));
    dev->dig_H5 = (int16_t)((((int16_t)calib2[5]) << 4) | (calib2[4] >> 4));
    dev->dig_H6 = (int8_t)calib2[6];

    // Set oversampling & mode:
    // humidity oversampling x1
    (void)wr8(dev, REG_CTRL_HUM, 0x01);
    // temp oversampling x1 (001), press x1 (001), mode normal (11) => 0b00100111 = 0x27
    (void)wr8(dev, REG_CTRL_MEAS, 0x27);
    // IIR filter off (000), standby 500ms (101) => 0b10100000 = 0xA0
    (void)wr8(dev, REG_CONFIG, 0xA0);

    return BME280_OK;
}// eo bme280_init::

// Fixed-point compensation per datasheet
/* compensate_T:=======================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Compensates the raw temperature reading from the BME280 sensor using calibration data.
 * Inputs: *dev - Pointer to BME280_Handle structure, adc_T - Raw temperature reading
 * Returns: Compensated temperature in °C * 100
 */
static int32_t compensate_T(BME280_Handle *dev, int32_t adc_T)
{
    int32_t var1  = ((((adc_T >> 3) - ((int32_t)dev->dig_T1 << 1))) * ((int32_t)dev->dig_T2)) >> 11;
    int32_t var2  = (((((adc_T >> 4) - ((int32_t)dev->dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->dig_T1))) >> 12) * ((int32_t)dev->dig_T3)) >> 14;
    dev->t_fine = var1 + var2;
    return (dev->t_fine * 5 + 128) >> 8; // °C *100
}// eo compensate_T::

/* compensate_P:=======================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Compensates the raw pressure reading from the BME280 sensor using calibration data.
 * Inputs: *dev - Pointer to BME280_Handle structure, adc_P - Raw pressure reading
 * Returns: Compensated pressure in Pa (Q24.8 format)
 */
static uint32_t compensate_P(BME280_Handle *dev, int32_t adc_P)
{
    int64_t var1 = ((int64_t)dev->t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dev->dig_P6;
    var2 = var2 + ((var1 * (int64_t)dev->dig_P5) << 17);
    var2 = var2 + (((int64_t)dev->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dev->dig_P3) >> 8) + ((var1 * (int64_t)dev->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->dig_P1) >> 33;
    if (var1 == 0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dev->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dev->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dev->dig_P7) << 4);
    return (uint32_t)p; // Q24.8 Pa
}// eo compensate_P::

// Fixed-point humidity compensation (datasheet)
/* compensate_H:=======================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Compensates the raw humidity reading from the BME280 sensor using calibration data.
 * Inputs: *dev - Pointer to BME280_Handle structure, adc_H - Raw humidity reading
 * Returns: Compensated humidity in %RH (Q22.10 format)
 */
static uint32_t compensate_H(BME280_Handle *dev, int32_t adc_H)
{
    int32_t v_x1_u32r;

    v_x1_u32r = dev->t_fine - ((int32_t)76800);

    v_x1_u32r =
        (((((adc_H << 14)
          - (((int32_t)dev->dig_H4) << 20)
          - (((int32_t)dev->dig_H5) * v_x1_u32r))
          + ((int32_t)16384)) >> 15)
        *
        (((((((v_x1_u32r * ((int32_t)dev->dig_H6)) >> 10)
            * (((v_x1_u32r * ((int32_t)dev->dig_H3)) >> 11) + ((int32_t)32768))) >> 10)
          + ((int32_t)2097152))
          * ((int32_t)dev->dig_H2)
          + ((int32_t)8192)) >> 14));

    v_x1_u32r =
        v_x1_u32r
        - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
           * ((int32_t)dev->dig_H1)) >> 4);

    if (v_x1_u32r < 0)
        v_x1_u32r = 0;
    if (v_x1_u32r > 419430400)
        v_x1_u32r = 419430400;

    return (uint32_t)(v_x1_u32r >> 12); /* Q22.10 -> %RH */
}// eo compensate_H::

/* bme280_force_measurement:===========================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Triggers a forced measurement on the BME280 sensor, which is useful when the sensor is in forced mode( not continuous).
 * Inputs: *dev - Pointer to BME280_Handle structure
 * Returns: BME280_Status indicating success or error
 */
BME280_Status bme280_force_measurement(BME280_Handle *dev)
{
    // Set forced mode (01) while keeping oversampling x1/x1
    if (wr8(dev, REG_CTRL_MEAS, 0x25) != HAL_OK) return BME280_ERR;
    return BME280_OK;
}

/* bme280_read_all:====================================================================
 * Author: OpenAI's ChatGPT, adapted by Vraj Patel
 * Date: 2025-11-01
 * Modified: None
 * Description: Reads temperature, pressure, and humidity data from the BME280 sensor and applies compensation.
 * Inputs: *dev - Pointer to BME280_Handle structure, *temperature_c - Pointer to store temperature in °C, *pressure_pa - Pointer to store pressure in Pa, *humidity_rh - Pointer to store humidity in %RH
 * Returns: BME280_Status indicating success or error
 */
BME280_Status bme280_read_all(BME280_Handle *dev, float *temperature_c, float *pressure_pa, float *humidity_rh)
{
    uint8_t data[8];
    // Read measurement registers: 0xF7..0xFE (P[19:0], T[19:0], H[15:0])
    if (rd(dev, REG_PRESS_MSB, data, 8) != HAL_OK) return BME280_ERR;

    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);
    int32_t adc_H = ((int32_t)data[6] << 8)  |  (int32_t)data[7];

    int32_t temp_c_x100 = compensate_T(dev, adc_T);
    uint32_t press_q24_8 = compensate_P(dev, adc_P);
    uint32_t hum_q22_10  = compensate_H(dev, adc_H);

    if (temperature_c) *temperature_c = temp_c_x100 / 100.0f;
    if (pressure_pa)   *pressure_pa   = press_q24_8 / 256.0f;     // Pa
    if (humidity_rh)   *humidity_rh   = hum_q22_10 / 1024.0f;     // %RH

    return BME280_OK;
}// eo bme280_read_all::
