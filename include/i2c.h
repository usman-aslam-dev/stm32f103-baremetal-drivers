/** @file i2c.h  @brief Blocking I2C1 master (PB6 SCL / PB7 SDA), 100 or 400 kHz. */
#ifndef I2C_H
#define I2C_H
#include <stdint.h>
#include <stddef.h>

typedef enum {
    I2C_OK = 0,
    I2C_ERR_TIMEOUT  = -1,
    I2C_ERR_NACK     = -2,   /* slave did not acknowledge */
    I2C_ERR_BUS      = -3,   /* BERR / ARLO */
} i2c_status_t;

void         i2c1_init(uint32_t pclk1_hz, uint32_t scl_hz);
i2c_status_t i2c1_write(uint8_t addr7, const uint8_t *data, size_t n);
i2c_status_t i2c1_read(uint8_t addr7, uint8_t *data, size_t n);
i2c_status_t i2c1_probe(uint8_t addr7);
void         i2c1_bus_recover(void);   /* 9 SCL pulses to unstick a wedged slave */

/* Pure helpers - host testable */
uint32_t i2c_calc_ccr_sm(uint32_t pclk1_hz, uint32_t scl_hz);
uint32_t i2c_calc_ccr_fm(uint32_t pclk1_hz, uint32_t scl_hz);
uint32_t i2c_calc_trise_sm(uint32_t pclk1_hz);
#endif
