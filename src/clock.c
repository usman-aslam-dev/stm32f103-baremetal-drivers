/**
 * @file  clock.c
 * @brief Bring SYSCLK to 72 MHz using the 8 MHz clock the ST-LINK feeds us.
 *
 * The Nucleo-F103RB has no crystal populated. HSE comes from the ST-LINK MCU's
 * MCO pin as a plain 8 MHz square wave, which is why HSEBYP must be set: bypass
 * tells the oscillator not to try to drive a resonator, just sample the pin.
 * Forgetting HSEBYP is the classic "HSERDY never sets" bug on this board.
 *
 * Order matters. Flash wait states go up BEFORE the clock does, never after,
 * or the core starts fetching faster than the flash can answer and you land in
 * a HardFault before main().
 */
#include "clock.h"
#include "stm32f103.h"

int clock_init_72mhz(void)
{
    /* 1. Start HSE in bypass mode and wait for it. */
    RCC->CR |= RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;

    uint32_t spins = 100000UL;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
        if (--spins == 0U) {
            return -1;   /* caller falls back to the 8 MHz HSI */
        }
    }

    /* 2. Flash: 2 wait states are mandatory above 48 MHz. Prefetch on to claw
     *    back some of the cost of those wait states. */
    FLASHR->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    /* 3. Bus prescalers, set before the PLL is selected so no bus is ever
     *    briefly overclocked during the switch. */
    RCC->CFGR &= ~(0xFUL << RCC_CFGR_HPRE_Pos);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1      /* AHB  = 72 MHz */
              |  RCC_CFGR_PPRE1_DIV2     /* APB1 = 36 MHz (hard ceiling)      */
              |  RCC_CFGR_PPRE2_DIV1     /* APB2 = 72 MHz                     */
              |  RCC_CFGR_ADCPRE_DIV6;   /* ADC  = 12 MHz (ceiling is 14 MHz) */

    /* 4. PLL: HSE undivided, x9 -> 8 * 9 = 72 MHz. */
    RCC->CFGR &= ~((0xFUL << RCC_CFGR_PLLMULL_Pos) | RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE);
    RCC->CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9;

    RCC->CR |= RCC_CR_PLLON;
    spins = 100000UL;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
        if (--spins == 0U) {
            return -1;
        }
    }

    /* 5. Switch, then confirm via SWS. Writing SW is a request, not a fact. */
    RCC->CFGR &= ~RCC_CFGR_SW_Msk;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    spins = 100000UL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
        if (--spins == 0U) {
            return -1;
        }
    }

    return 0;
}
