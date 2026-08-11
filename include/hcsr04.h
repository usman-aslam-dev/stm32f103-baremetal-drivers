/** @file hcsr04.h  @brief HC-SR04 echo width measured with TIM3 input capture. */
#ifndef HCSR04_H
#define HCSR04_H
#include <stdint.h>

#define HCSR04_NO_ECHO   0xFFFFFFFFUL

void     hcsr04_init(void);
uint32_t hcsr04_read_us(void);           /* echo pulse width, or HCSR04_NO_ECHO */
uint32_t hcsr04_us_to_mm(uint32_t us);   /* pure - host testable */
#endif
