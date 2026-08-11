/**
 * @file  fake_regs.c
 * @brief RAM stand-ins for the peripheral registers, used only on the host.
 *
 * This is the trick that makes register-level drivers unit testable. On the
 * target, RCC/GPIOB/I2C1 are macros pointing at real addresses. Compiled with
 * -DUNIT_TEST they become pointers to these plain structs instead, so a test can
 * call i2c1_init() on x86 and then assert on exactly which bits were written.
 *
 * No hardware, no board, no debugger - and it runs in CI on every push.
 */
#include "stm32f103.h"

static RCC_TypeDef    s_rcc;
static FLASH_TypeDef  s_flash;
static GPIO_TypeDef   s_gpioa, s_gpiob, s_gpioc;
static AFIO_TypeDef   s_afio;
static USART_TypeDef  s_usart2;
static I2C_TypeDef    s_i2c1;
static ADC_TypeDef    s_adc1;
static TIM_TypeDef    s_tim3;

RCC_TypeDef   *const RCC    = &s_rcc;
FLASH_TypeDef *const FLASHR = &s_flash;
GPIO_TypeDef  *const GPIOA  = &s_gpioa;
GPIO_TypeDef  *const GPIOB  = &s_gpiob;
GPIO_TypeDef  *const GPIOC  = &s_gpioc;
AFIO_TypeDef  *const AFIO   = &s_afio;
USART_TypeDef *const USART2 = &s_usart2;
I2C_TypeDef   *const I2C1   = &s_i2c1;
ADC_TypeDef   *const ADC1   = &s_adc1;
TIM_TypeDef   *const TIM3   = &s_tim3;

void fake_regs_reset(void)
{
    RCC_TypeDef    z_rcc    = {0}; s_rcc    = z_rcc;
    GPIO_TypeDef   z_gpio   = {0}; s_gpioa  = z_gpio; s_gpiob = z_gpio; s_gpioc = z_gpio;
    USART_TypeDef  z_usart  = {0}; s_usart2 = z_usart;
    I2C_TypeDef    z_i2c    = {0}; s_i2c1   = z_i2c;
    ADC_TypeDef    z_adc    = {0}; s_adc1   = z_adc;
    TIM_TypeDef    z_tim    = {0}; s_tim3   = z_tim;
}
