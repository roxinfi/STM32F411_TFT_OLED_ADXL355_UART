#ifndef BME280_H
#define BME280_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;     // 0x76 or 0x77
    // Calibration coefficients
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
    // private
    int32_t  t_fine;
} BME280_Handle;

/* Addresses */
#define BME280_I2C_ADDR_0x76   (0x76 << 1)
#define BME280_I2C_ADDR_0x77   (0x77 << 1)

/* Return codes */
typedef enum {
    BME280_OK = 0,
    BME280_ERR = -1,
    BME280_ERR_ID = -2
} BME280_Status;

/* Public API */
BME280_Status bme280_init(BME280_Handle *dev, I2C_HandleTypeDef *hi2c, uint8_t i2c_addr);
BME280_Status bme280_read_all(BME280_Handle *dev, float *temperature_c, float *pressure_pa, float *humidity_rh);
BME280_Status bme280_force_measurement(BME280_Handle *dev); // optional if using forced mode

#ifdef __cplusplus
}
#endif
#endif /* BME280_H */
