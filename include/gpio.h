/** @file gpio.h  @brief Thin wrapper over the F1 CRL/CRH pin configuration model. */
#ifndef GPIO_H
#define GPIO_H
#include "stm32f103.h"

/** Apply one of the GPIO_CFG_* nibbles to a pin (0..15). */
void gpio_configure(GPIO_TypeDef *port, uint8_t pin, uint8_t cfg_nibble);

/** Atomic set / clear via BSRR - never a read-modify-write on ODR. */
static inline void gpio_set(GPIO_TypeDef *port, uint8_t pin)   { port->BSRR = (1UL << pin); }
static inline void gpio_clear(GPIO_TypeDef *port, uint8_t pin) { port->BSRR = (1UL << (pin + 16)); }
static inline void gpio_write(GPIO_TypeDef *port, uint8_t pin, int v)
{
    port->BSRR = v ? (1UL << pin) : (1UL << (pin + 16));
}
static inline int  gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR >> pin) & 1U;
}
#endif
