/** @file clock.h  @brief System clock tree: 72 MHz from the 8 MHz ST-LINK MCO. */
#ifndef CLOCK_H
#define CLOCK_H
#include <stdint.h>

#define SYSCLK_HZ   72000000UL
#define HCLK_HZ     72000000UL
#define PCLK1_HZ    36000000UL   /* APB1 - hard limit 36 MHz */
#define PCLK2_HZ    72000000UL   /* APB2 */
#define ADCCLK_HZ   12000000UL   /* PCLK2 / 6 - hard limit 14 MHz */

/** Configure PLL for 72 MHz. Returns 0 on success, -1 if HSE never became ready. */
int clock_init_72mhz(void);
#endif
