/**
 * @file  systick.c
 * @brief 1 kHz system tick.
 *
 * CLKSOURCE = 1 runs SysTick from HCLK directly rather than HCLK/8, so the
 * reload value is just "cycles per millisecond minus one".
 */
#include "systick.h"
#include "stm32f103.h"

static volatile uint32_t s_ticks;

void systick_init_1khz(uint32_t hclk_hz)
{
    SysTickR->LOAD = (hclk_hz / 1000U) - 1U;
    SysTickR->VAL  = 0;
    SysTickR->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
}

void SysTick_Handler(void)
{
    s_ticks++;
}

uint32_t millis(void)
{
    return s_ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    /* Unsigned subtraction so this stays correct across the 49-day wrap. */
    while ((s_ticks - start) < ms) { }
}
