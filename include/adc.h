/** @file adc.h  @brief ADC1 with the internal temperature sensor and VREFINT. */
#ifndef ADC_H
#define ADC_H
#include <stdint.h>

void     adc1_init(void);
uint16_t adc1_read_channel(uint8_t channel);
uint16_t adc1_read_vrefint(void);
uint16_t adc1_read_temp_raw(void);

/** True VDDA in millivolts, derived from the 1.20 V internal reference. */
uint32_t adc_calc_vdda_mv(uint16_t vrefint_raw);

/**
 * Die temperature in milli-degrees C.
 * Uncalibrated absolute accuracy is roughly +/-45 C - this is a trend sensor.
 */
int32_t  adc_calc_temp_mdeg(uint16_t temp_raw, uint32_t vdda_mv);
#endif
