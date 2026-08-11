/**
 * @file  test_drivers.c
 * @brief Host unit tests. No board required - these run in CI on every push.
 *
 * Two categories:
 *   1. Pure maths (BRR, CCR, TRISE, temperature, distance) - checked against the
 *      worked examples in the reference manual.
 *   2. Register effects - call a driver against the fake register bank and assert
 *      the exact bits it set. This is what catches "I configured the wrong pin"
 *      without owning a logic analyser.
 */
#include <stdio.h>
#include <string.h>
#include "stm32f103.h"
#include "uart.h"
#include "i2c.h"
#include "adc.h"
#include "gpio.h"
#include "hcsr04.h"

void fake_regs_reset(void);

static int g_fail;
static int g_run;

#define CHECK(cond, fmt, ...)                                        \
    do {                                                             \
        g_run++;                                                     \
        if (!(cond)) {                                               \
            g_fail++;                                                \
            printf("  FAIL %s:%d  " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
        }                                                            \
    } while (0)

/* -- 1. Pure maths ------------------------------------------------------- */

static void test_uart_brr(void)
{
    /* RM0008 worked example: 36 MHz, 115200 -> mantissa 19, fraction 8. */
    CHECK(uart_calc_brr(36000000, 115200) == 0x138,
          "36MHz/115200 gave 0x%lX, expected 0x138",
          (unsigned long)uart_calc_brr(36000000, 115200));

    /* 36 MHz, 9600 -> 234.375 -> mantissa 234 (0xEA), fraction 6 */
    CHECK(uart_calc_brr(36000000, 9600) == 0xEA6, "9600 baud divisor wrong");
}

static void test_i2c_timing(void)
{
    /* Standard mode 100 kHz at 36 MHz: CCR = 36e6/(2*100e3) = 180 */
    CHECK(i2c_calc_ccr_sm(36000000, 100000) == 180, "CCR Sm should be 180");

    /* Fast mode 400 kHz, DUTY=0: CCR = 36e6/(3*400e3) = 30 */
    CHECK(i2c_calc_ccr_fm(36000000, 400000) == 30, "CCR Fm should be 30");

    /* TRISE = FREQ + 1 = 37 */
    CHECK(i2c_calc_trise_sm(36000000) == 37, "TRISE should be 37");

    /* RM0008 forbids CCR below 4 in standard mode. */
    CHECK(i2c_calc_ccr_sm(2000000, 400000) >= 4, "CCR must be clamped to >= 4");
}

static void test_adc_maths(void)
{
    /* If VREFINT (1.20 V) reads mid-scale, VDDA must be about 2.40 V. */
    uint32_t vdda = adc_calc_vdda_mv(2048);
    CHECK(vdda > 2380 && vdda < 2420, "VDDA from mid-scale VREFINT = %lu", (unsigned long)vdda);

    /* A perfect 3.3 V rail puts VREFINT at 1.20/3.30 * 4095 = 1489 counts. */
    vdda = adc_calc_vdda_mv(1489);
    CHECK(vdda > 3280 && vdda < 3320, "VDDA from 1489 counts = %lu", (unsigned long)vdda);

    /* Vsense = V25 = 1.43 V must come back as 25 C. At 3.3 V that is
     * 1430/3300 * 4095 = 1775 counts. */
    int32_t t = adc_calc_temp_mdeg(1775, 3300);
    CHECK(t > 24000 && t < 26000, "V25 should read ~25 C, got %ld mdeg", (long)t);

    /* The sensor has a negative coefficient: a higher voltage means colder. */
    CHECK(adc_calc_temp_mdeg(1900, 3300) < adc_calc_temp_mdeg(1775, 3300),
          "temperature should fall as Vsense rises");
}

static void test_hcsr04_maths(void)
{
    /* 5831 us round trip is 1 metre. */
    uint32_t mm = hcsr04_us_to_mm(5831);
    CHECK(mm > 990 && mm < 1010, "5831 us should be ~1000 mm, got %lu", (unsigned long)mm);

    CHECK(hcsr04_us_to_mm(HCSR04_NO_ECHO) == HCSR04_NO_ECHO, "timeout must pass through");
}

/* -- 2. Register-level effects ------------------------------------------- */

static void test_gpio_nibbles(void)
{
    fake_regs_reset();

    /* Pin 5 lives in CRL, nibble 5 -> bits 23:20. */
    gpio_configure(GPIOA, 5, GPIO_CFG_OUT_PP_2MHZ);
    CHECK(((GPIOA->CRL >> 20) & 0xF) == GPIO_CFG_OUT_PP_2MHZ, "PA5 nibble misplaced");
    CHECK(GPIOA->CRH == 0, "configuring pin 5 must not touch CRH");

    /* Pin 11 lives in CRH, nibble 3 -> bits 15:12. */
    fake_regs_reset();
    gpio_configure(GPIOA, 11, GPIO_CFG_AF_PP_50MHZ);
    CHECK(((GPIOA->CRH >> 12) & 0xF) == GPIO_CFG_AF_PP_50MHZ, "PA11 nibble misplaced");
    CHECK(GPIOA->CRL == 0, "configuring pin 11 must not touch CRL");

    /* Reconfiguring must replace the nibble, not OR into it. */
    gpio_configure(GPIOA, 11, GPIO_CFG_IN_FLOATING);
    CHECK(((GPIOA->CRH >> 12) & 0xF) == GPIO_CFG_IN_FLOATING, "old nibble not cleared");
}

static void test_gpio_bsrr_is_atomic(void)
{
    fake_regs_reset();

    gpio_set(GPIOA, 5);
    CHECK(GPIOA->BSRR == (1u << 5), "set should write the low BSRR half");

    gpio_clear(GPIOA, 5);
    CHECK(GPIOA->BSRR == (1u << 21), "clear should write the high BSRR half");

    /* ODR must never be touched - that would be a read-modify-write race. */
    CHECK(GPIOA->ODR == 0, "driver must not read-modify-write ODR");
}

static void test_i2c_init_uses_open_drain(void)
{
    fake_regs_reset();
    i2c1_init(36000000, 100000);

    /* PB6 and PB7 are nibbles 6 and 7 of CRL. Push-pull here is the classic
     * bring-up bug, so it is pinned down by a test. */
    CHECK(((GPIOB->CRL >> 24) & 0xF) == GPIO_CFG_AF_OD_50MHZ, "PB6 must be AF open-drain");
    CHECK(((GPIOB->CRL >> 28) & 0xF) == GPIO_CFG_AF_OD_50MHZ, "PB7 must be AF open-drain");

    CHECK((I2C1->CR2 & 0x3F) == 36, "CR2 FREQ must be PCLK1 in MHz");
    CHECK(I2C1->CCR == 180, "CCR wrong");
    CHECK(I2C1->TRISE == 37, "TRISE wrong");
    CHECK(I2C1->CR1 & I2C_CR1_PE, "peripheral left disabled");

    CHECK(RCC->APB1ENR & RCC_APB1ENR_I2C1EN, "I2C1 clock not enabled");
    CHECK(RCC->APB2ENR & RCC_APB2ENR_IOPBEN, "GPIOB clock not enabled");
}

static void test_uart_init_pins(void)
{
    fake_regs_reset();
    uart2_init(115200);

    /* PA2 = TX must be alternate-function push-pull; PA3 = RX must be an input. */
    CHECK(((GPIOA->CRL >> 8)  & 0xF) == GPIO_CFG_AF_PP_50MHZ, "PA2 must be AF push-pull");
    CHECK(((GPIOA->CRL >> 12) & 0xF) == GPIO_CFG_IN_FLOATING, "PA3 must be floating input");

    CHECK(USART2->BRR == 0x138, "BRR not programmed for 115200 at 36 MHz");
    CHECK(USART2->CR1 & USART_CR1_UE, "USART left disabled");
    CHECK(USART2->CR1 & USART_CR1_TE, "TX not enabled");
    CHECK(USART2->CR1 & USART_CR1_RE, "RX not enabled");
}

int main(void)
{
    printf("STM32F103 driver unit tests (host)\n");

    test_uart_brr();
    test_i2c_timing();
    test_adc_maths();
    test_hcsr04_maths();
    test_gpio_nibbles();
    test_gpio_bsrr_is_atomic();
    test_i2c_init_uses_open_drain();
    test_uart_init_pins();

    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
