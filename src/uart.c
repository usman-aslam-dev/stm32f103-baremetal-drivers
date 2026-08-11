/**
 * @file  uart.c
 * @brief USART2 on PA2/PA3, which the Nucleo routes to the ST-LINK USB serial port.
 *
 * No wires needed: plug the board in, open /dev/ttyACM0 at 115200 and you have a
 * console. Solder bridges SB13/SB14 do this on the board (UM1724).
 */
#include "uart.h"
#include "gpio.h"
#include "clock.h"
#include "stm32f103.h"

/**
 * BRR holds a 12-bit mantissa and a 4-bit fraction of USARTDIV.
 * USARTDIV = PCLK / (16 * baud), so PCLK / baud is already USARTDIV * 16 -
 * which is exactly the layout BRR wants. No shifting games required.
 *
 * 36 MHz, 115200 -> 312.5 -> truncate to 312 -> mantissa 19, fraction 8 -> 0x138.
 *
 * Note the tie: the true value is 312.5, exactly between two representable
 * settings. Truncating gives 0x138 (+0.16% baud error); rounding up gives 0x139
 * (-0.16%). Both work on a real link, but truncation is what ST's own HAL and
 * the RM0008 worked examples produce, so this matches the reference rather than
 * inventing a third answer. Verified by test_uart_brr().
 */
uint32_t uart_calc_brr(uint32_t pclk_hz, uint32_t baud)
{
    return pclk_hz / baud;   /* already scaled by 16 - mantissa:fraction */
}

void uart2_init(uint32_t baud)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_configure(GPIOA, 2, GPIO_CFG_AF_PP_50MHZ);   /* TX driven by the periph */
    gpio_configure(GPIOA, 3, GPIO_CFG_IN_FLOATING);   /* RX is an input          */

    USART2->BRR = uart_calc_brr(PCLK1_HZ, baud);
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;  /* 8N1 */
    USART2->CR2 = 0;
    USART2->CR3 = 0;
}

void uart2_write_byte(uint8_t b)
{
    while (!(USART2->SR & USART_SR_TXE)) { }
    USART2->DR = b;
}

void uart2_write(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart2_write_byte('\r');   /* terminals expect CRLF */
        }
        uart2_write_byte((uint8_t)*s++);
    }
}

void uart2_write_len(const uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uart2_write_byte(buf[i]);
    }
}

int uart2_read_byte_nonblocking(uint8_t *out)
{
    uint32_t sr = USART2->SR;

    /* Overrun must be cleared by reading SR then DR, otherwise RXNE never
     * asserts again and the link appears dead. */
    if (sr & USART_SR_ORE) {
        (void)USART2->DR;
        return 0;
    }
    if (sr & USART_SR_RXNE) {
        *out = (uint8_t)USART2->DR;
        return 1;
    }
    return 0;
}
