/**
 * @file  dwt_delay.c
 * @brief Sub-microsecond delays using the Cortex-M3 data watchpoint cycle counter.
 *
 * DWT_CYCCNT is a free-running count of core clock cycles. It costs nothing to
 * read and is exact, which makes it far better than a calibrated NOP loop for
 * the 10 us HC-SR04 trigger pulse and the 100 us HD44780 setup delays.
 *
 * TRCENA in DEMCR must be set first or the DWT block stays powered down and
 * CYCCNT reads back as a constant zero - a confusing failure the first time.
 */
#include "dwt_delay.h"
#include "clock.h"
#include "stm32f103.h"

void dwt_init(void)
{
    DEMCR |= DEMCR_TRCENA;
    DWTR->CYCCNT = 0;
    DWTR->CTRL |= DWT_CTRL_CYCCNTENA;
}

uint32_t dwt_cycles(void)
{
    return DWTR->CYCCNT;
}

void delay_us(uint32_t us)
{
    uint32_t start  = DWTR->CYCCNT;
    uint32_t target = us * (SYSCLK_HZ / 1000000U);
    while ((DWTR->CYCCNT - start) < target) { }
}
