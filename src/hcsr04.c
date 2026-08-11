/**
 * @file  hcsr04.c
 * @brief HC-SR04 ultrasonic range finder via TIM3 input capture.
 *
 * The sensor answers a 10 us trigger pulse with an echo pulse whose width is
 * proportional to distance. Measuring that width by polling a GPIO would be at
 * the mercy of interrupt latency, so instead TIM3 captures the timestamps of
 * both edges in hardware and software only subtracts them.
 *
 * PSC = 71 makes the timer tick once per microsecond at 72 MHz, so the captured
 * difference is directly in microseconds and no scaling maths is needed.
 *
 * Wiring note: the HC-SR04 echo pin is 5 V. Use a divider (e.g. 1k/2k) into
 * PA6 - the STM32 pin is not 5 V tolerant in analog-capable configurations.
 */
#include "hcsr04.h"
#include "gpio.h"
#include "dwt_delay.h"
#include "systick.h"
#include "stm32f103.h"

#define TRIG_PORT   GPIOA
#define TRIG_PIN    7
#define ECHO_PIN    6          /* PA6 = TIM3_CH1 */

#define ECHO_TIMEOUT_MS  30    /* ~4 m of round trip is about 23 ms */

void hcsr04_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    gpio_configure(TRIG_PORT, TRIG_PIN, GPIO_CFG_OUT_PP_50MHZ);
    gpio_configure(TRIG_PORT, ECHO_PIN, GPIO_CFG_IN_FLOATING);

    TIM3->PSC = 71;            /* 72 MHz / (71+1) = 1 MHz -> 1 us per tick */
    TIM3->ARR = 0xFFFF;

    /* CH1 captures rising edges, CH2 is fed from the same pin (TI1) but
     * captures falling edges. Two captures, one input - PWM input mode. */
    TIM3->CCMR1 = (1UL << 0)    /* CC1S = 01: IC1 mapped to TI1 */
                | (2UL << 8);   /* CC2S = 10: IC2 mapped to TI1 */
    TIM3->CCER  = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC2P;  /* CC2 falling */

    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 = TIM_CR1_CEN;
}

uint32_t hcsr04_read_us(void)
{
    TIM3->SR = 0;

    /* 10 us trigger, exactly as the datasheet asks. */
    gpio_set(TRIG_PORT, TRIG_PIN);
    delay_us(10);
    gpio_clear(TRIG_PORT, TRIG_PIN);

    uint32_t start = millis();

    while (!(TIM3->SR & TIM_SR_CC1IF)) {
        if ((millis() - start) > ECHO_TIMEOUT_MS) return HCSR04_NO_ECHO;
    }
    uint32_t rise = TIM3->CCR[0];

    while (!(TIM3->SR & TIM_SR_CC2IF)) {
        if ((millis() - start) > ECHO_TIMEOUT_MS) return HCSR04_NO_ECHO;
    }
    uint32_t fall = TIM3->CCR[1];

    /* 16-bit counter, so the subtraction is masked to handle one wrap. */
    return (fall - rise) & 0xFFFFU;
}

/**
 * Sound travels ~343 m/s at 20 C. The pulse covers the distance twice, so
 *   mm = us * 343 / 1000 / 2  ~=  us * 1000 / 5831
 * Integer form avoids floating point. Accuracy degrades with air temperature
 * by roughly 0.6 m/s per degree - stated in the README rather than corrected,
 * because there is no calibrated thermometer in this build to correct with.
 */
uint32_t hcsr04_us_to_mm(uint32_t us)
{
    if (us == HCSR04_NO_ECHO) return HCSR04_NO_ECHO;
    return (us * 1000U) / 5831U;
}
