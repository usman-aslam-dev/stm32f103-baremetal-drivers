/** @file lcd_pcf8574.h  @brief HD44780 16x2 over a PCF8574 I2C backpack, 4-bit mode. */
#ifndef LCD_PCF8574_H
#define LCD_PCF8574_H
#include <stdint.h>
#include "i2c.h"

/* PCF8574 = 0x20..0x27, PCF8574A = 0x38..0x3F. Most cheap backpacks are 0x27. */
#define LCD_ADDR_DEFAULT   0x27

i2c_status_t lcd_init(uint8_t addr7);
void         lcd_clear(void);
void         lcd_set_cursor(uint8_t row, uint8_t col);
void         lcd_print(const char *s);
void         lcd_backlight(int on);
#endif
