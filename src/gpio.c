/**
 * @file  gpio.c
 * @brief The F1 pin configuration model in one function.
 *
 * STM32F1 predates the MODER/OTYPER/OSPEEDR/PUPDR split of the F4 and L4 parts.
 * Instead each pin gets a single 4-bit nibble, and the nibbles for pins 0-7 live
 * in CRL while 8-15 live in CRH. So configuring pin 11 means editing bits 15:12
 * of CRH, not CRL. That index arithmetic is the entire function.
 */
#include "gpio.h"

void gpio_configure(GPIO_TypeDef *port, uint8_t pin, uint8_t cfg_nibble)
{
    volatile uint32_t *reg = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin % 8U) * 4U;

    uint32_t v = *reg;
    v &= ~(0xFUL << shift);                       /* clear the old nibble  */
    v |= ((uint32_t)cfg_nibble & 0xFUL) << shift; /* drop the new one in   */
    *reg = v;
}
