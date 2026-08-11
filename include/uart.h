/** @file uart.h  @brief USART2 on PA2/PA3 - wired to the Nucleo ST-LINK virtual COM port. */
#ifndef UART_H
#define UART_H
#include <stdint.h>
#include <stddef.h>

void uart2_init(uint32_t baud);
void uart2_write_byte(uint8_t b);
void uart2_write(const char *s);
void uart2_write_len(const uint8_t *buf, size_t n);
int  uart2_read_byte_nonblocking(uint8_t *out);   /* 1 = got a byte, 0 = none */

/** Compute BRR for oversampling-by-16. Pure function -> unit testable on host. */
uint32_t uart_calc_brr(uint32_t pclk_hz, uint32_t baud);
#endif
