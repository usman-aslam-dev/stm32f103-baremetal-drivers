/**
 * @file  adc.c
 * @brief ADC1 driving the internal temperature sensor and voltage reference.
 *
 * There is no external I2C sensor in this build, so the die temperature sensor
 * on channel 16 is the physical quantity being measured. It is a genuine analog
 * sensor with a genuine calibration problem, which makes it a better teaching
 * example than a module that hands you a pre-cooked number.
 *
 * Honest accuracy note: V25 and Avg_Slope are *typical* values with wide
 * production spread, so uncalibrated absolute error is around +/-45 C. Relative
 * change is repeatable to well under a degree. This driver reports the number
 * and the README states the caveat rather than pretending it is a thermometer.
 *
 * VREFINT on channel 17 is the useful trick: it is a fixed 1.20 V bandgap, so
 * measuring it backwards tells you what VDDA actually is, letting every other
 * reading be corrected for supply drift.
 */
#include "adc.h"
#include "clock.h"
#include "dwt_delay.h"
#include "stm32f103.h"

#define VREFINT_MV      1200U    /* datasheet typical */
#define ADC_FULL_SCALE  4095U
#define V25_MV          1430     /* temp sensor output at 25 C, typical */
#define AVG_SLOPE_UV    4300     /* microvolts per degree C, typical, negative TC */

void adc1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* The temperature sensor needs a long acquisition window - 17.1 us minimum.
     * At 12 MHz ADCCLK the 239.5-cycle setting is ~20 us, comfortably over. */
    ADC1->SMPR1 |= (uint32_t)ADC_SMP_239CYC << (3U * (ADC_CH_TEMP    - 10U));
    ADC1->SMPR1 |= (uint32_t)ADC_SMP_239CYC << (3U * (ADC_CH_VREFINT - 10U));

    /* TSVREFE connects the sensor and the reference to their channels. Without
     * it channels 16 and 17 read garbage. */
    ADC1->CR2 |= ADC_CR2_TSVREFE;

    /* Software trigger for regular conversions. */
    ADC1->CR2 |= ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG;

    /* First ADON write only wakes the ADC out of power-down; the datasheet asks
     * for a stabilisation time before it will convert. */
    ADC1->CR2 |= ADC_CR2_ADON;
    delay_us(10);

    /* Calibration removes the internal capacitor offset error. Both bits are
     * hardware-cleared when their operation completes. */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) { }
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) { }
}

uint16_t adc1_read_channel(uint8_t channel)
{
    ADC1->SQR1 = 0;                       /* L = 0 -> one conversion in sequence */
    ADC1->SQR3 = channel & 0x1FU;         /* SQ1 = this channel                  */

    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC)) { }

    return (uint16_t)(ADC1->DR & 0x0FFFU);   /* reading DR clears EOC */
}

uint16_t adc1_read_vrefint(void)  { return adc1_read_channel(ADC_CH_VREFINT); }
uint16_t adc1_read_temp_raw(void) { return adc1_read_channel(ADC_CH_TEMP); }

/**
 * VREFINT is a known 1.20 V. If it reads as `raw` counts out of 4095, then
 *   VDDA = 1.20 V * 4095 / raw
 * This is how you get a real supply voltage without an external reference.
 */
uint32_t adc_calc_vdda_mv(uint16_t vrefint_raw)
{
    if (vrefint_raw == 0U) return 0U;
    return (VREFINT_MV * ADC_FULL_SCALE) / vrefint_raw;
}

/**
 * RM0008:  T = (V25 - Vsense) / Avg_Slope + 25
 * Returned in milli-degrees to avoid floating point on a Cortex-M3 with no FPU.
 */
int32_t adc_calc_temp_mdeg(uint16_t temp_raw, uint32_t vdda_mv)
{
    int32_t vsense_uv = (int32_t)(((uint64_t)temp_raw * vdda_mv * 1000U) / ADC_FULL_SCALE);
    int32_t v25_uv    = V25_MV * 1000;
    return (((v25_uv - vsense_uv) * 1000) / AVG_SLOPE_UV) + 25000;
}
