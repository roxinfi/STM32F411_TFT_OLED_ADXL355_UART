#include "adxl335.h"
#include <math.h>
#include <string.h>

// ----- helpers -----
static inline float adc_to_volt(uint16_t adc)
{
    return ((float)adc * (VREF_MV / 1000.0f)) / (float)ADC_MAX_VALUE;
}

static inline float lpf(float prev, float in, float alpha)
{
    return prev + alpha * (in - prev);
}

void ADXL335_InitDefaults(ADXL335_State *s)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));

    s->sens_v_per_g = 0.300f;  // typical ADXL335 sensitivity @ 3.3V
    s->alpha        = 0.10f;   // moderate smoothing

    // PWM mapping
    s->dead_deg = 5.0f;        // below 5° => off
    s->max_deg  = 60.0f;       // above 60° => full bright

    // Assume flat at init (will be overwritten by calibrate)
    s->v0x   = (VREF_MV/1000.0f) * 0.5f;
    s->v0y   = (VREF_MV/1000.0f) * 0.5f;
    s->vz_1g = (VREF_MV/1000.0f) * 0.5f;

    s->fx = 0.0f;
    s->fy = 0.0f;
    s->fz = 1.0f;
}

// Keep the board still and flat during this calibration.
void ADXL335_CalibrateFlat(ADXL335_State *s, uint16_t ms)
{
    if (!s) return;

    uint32_t start = HAL_GetTick();
    float sx=0, sy=0, sz=0;
    uint32_t n=0;

    while ((HAL_GetTick() - start) < ms)
    {
        float vx = adc_to_volt(adc_dma_buffer[ADXL_X_INDEX]);
        float vy = adc_to_volt(adc_dma_buffer[ADXL_Y_INDEX]);
        float vz = adc_to_volt(adc_dma_buffer[ADXL_Z_INDEX]);

        sx += vx; sy += vy; sz += vz;
        n++;
        HAL_Delay(2);
    }

    if (n == 0) return;

    float mx = sx / (float)n;
    float my = sy / (float)n;
    float mz = sz / (float)n;

    // Flat calibration:
    // X,Y should be ~0g (store as 0g reference)
    // Z should be ~+1g (store as +1g reference)
    s->v0x   = mx;
    s->v0y   = my;
    s->vz_1g = mz;

    // Keep sensitivity default unless user wants to tune
    if (s->sens_v_per_g < 0.15f || s->sens_v_per_g > 0.60f)
        s->sens_v_per_g = 0.300f;

    // Reset filtered values
    s->fx = 0.0f;
    s->fy = 0.0f;
    s->fz = 1.0f;
}

void ADXL335_Update(ADXL335_State *s)
{
    if (!s) return;

    float vx = adc_to_volt(adc_dma_buffer[ADXL_X_INDEX]);
    float vy = adc_to_volt(adc_dma_buffer[ADXL_Y_INDEX]);
    float vz = adc_to_volt(adc_dma_buffer[ADXL_Z_INDEX]);

    // Convert to g using flat calibration references
    float ax = (vx - s->v0x) / s->sens_v_per_g;               // g
    float ay = (vy - s->v0y) / s->sens_v_per_g;               // g
    float az = 1.0f + (vz - s->vz_1g) / s->sens_v_per_g;      // g (flat => +1)

    // Filter
    const float a = s->alpha;
    s->fx = lpf(s->fx, ax, a);
    s->fy = lpf(s->fy, ay, a);
    s->fz = lpf(s->fz, az, a);
}

void ADXL335_GetAnglesDeg(const ADXL335_State *s, float *roll, float *pitch)
{
    if (!s) return;

    // roll  = atan2(y, z)
    // pitch = atan2(-x, sqrt(y^2+z^2))
    float r = atan2f(s->fy, s->fz) * 57.29578f;
    float p = atan2f(-s->fx, sqrtf(s->fy*s->fy + s->fz*s->fz)) * 57.29578f;

    if (roll)  *roll  = r;
    if (pitch) *pitch = p;
}

uint16_t ADXL335_TiltToPwmCcr(const ADXL335_State *s, uint32_t arr)
{
    if (!s) return 0;

    float roll, pitch;
    ADXL335_GetAnglesDeg(s, &roll, &pitch);

    float tilt = sqrtf(roll*roll + pitch*pitch);

    // Map tilt -> 0..1 brightness
    float brightness;
    if (tilt <= s->dead_deg) brightness = 0.0f;
    else if (tilt >= s->max_deg) brightness = 1.0f;
    else brightness = (tilt - s->dead_deg) / (s->max_deg - s->dead_deg);

    uint32_t ccr = (uint32_t)(brightness * (float)(arr + 1U));
    if (ccr > arr) ccr = arr;
    return (uint16_t)ccr;
}
