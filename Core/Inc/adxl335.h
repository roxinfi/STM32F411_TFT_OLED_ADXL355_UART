#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Calibration references
    float v0x;     // X 0g reference voltage
    float v0y;     // Y 0g reference voltage
    float vz_1g;   // Z +1g reference voltage when board is flat

    // Sensor sensitivity in V/g (ADXL335 ~ 0.300 V/g @ 3.3V)
    float sens_v_per_g;

    // Filtered acceleration values in g
    float fx;
    float fy;
    float fz;

    // LPF alpha (0.05 very smooth, 0.2 faster)
    float alpha;

    // PWM mapping thresholds in degrees
    float dead_deg;  // below this tilt => 0%
    float max_deg;   // above this tilt => 100%
} ADXL335_State;

// Configure these to match your DMA buffer indices
#ifndef ADXL_X_INDEX
#define ADXL_X_INDEX  0
#endif
#ifndef ADXL_Y_INDEX
#define ADXL_Y_INDEX  1
#endif
#ifndef ADXL_Z_INDEX
#define ADXL_Z_INDEX  2
#endif

// You already have these in your project:
#ifndef ADC_MAX_VALUE
#define ADC_MAX_VALUE 4095.0f
#endif
#ifndef VREF_MV
#define VREF_MV 3300.0f
#endif

// Provide your ADC DMA buffer and channel count externally
extern volatile uint16_t adc_dma_buffer[];

// API
void ADXL335_InitDefaults(ADXL335_State *s);
void ADXL335_CalibrateFlat(ADXL335_State *s, uint16_t ms);
void ADXL335_Update(ADXL335_State *s);

void ADXL335_GetAnglesDeg(const ADXL335_State *s, float *roll, float *pitch);
uint16_t ADXL335_TiltToPwmCcr(const ADXL335_State *s, uint32_t arr);

#ifdef __cplusplus
}
#endif
