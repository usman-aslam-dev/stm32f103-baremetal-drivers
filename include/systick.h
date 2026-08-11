/** @file systick.h  @brief 1 kHz tick + millisecond delays. */
#ifndef SYSTICK_H
#define SYSTICK_H
#include <stdint.h>
void     systick_init_1khz(uint32_t hclk_hz);
uint32_t millis(void);
void     delay_ms(uint32_t ms);
#endif
