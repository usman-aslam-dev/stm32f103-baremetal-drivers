/** @file dwt_delay.h  @brief Microsecond delay from the Cortex-M3 cycle counter. */
#ifndef DWT_DELAY_H
#define DWT_DELAY_H
#include <stdint.h>
void     dwt_init(void);
void     delay_us(uint32_t us);
uint32_t dwt_cycles(void);
#endif
