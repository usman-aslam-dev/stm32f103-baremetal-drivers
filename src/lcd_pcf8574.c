/**
 * @file  lcd_pcf8574.c
 * @brief HD44780 character LCD behind a PCF8574 I2C port expander.
 *
 * The backpack is an 8-bit I2C I/O expander with the LCD's control and upper
 * data pins wired to it:
 *
 *   P7 P6 P5 P4   P3   P2  P1  P0
 *   D7 D6 D5 D4   BL   E   RW  RS
 *
 * So every LCD access is really "write one byte to a PCF8574", and because only
 * four data lines are connected, each 8-bit LCD command is sent as two nibbles,
 * each latched by pulsing E high then low.
 *
 * The power-on sequence is not optional folklore - the HD44780 boots in 8-bit
 * mode and has to be walked into 4-bit mode with specific delays before it will
 * accept anything else.
 */
#include "lcd_pcf8574.h"
#include "dwt_delay.h"
#include "systick.h"

#define PIN_RS   0x01
#define PIN_RW   0x02
#define PIN_EN   0x04
#define PIN_BL   0x08

static uint8_t s_addr;
static uint8_t s_backlight = PIN_BL;

static i2c_status_t expander_write(uint8_t value)
{
    uint8_t b = value | s_backlight;
    return i2c1_write(s_addr, &b, 1);
}

/** Latch whatever is on the data lines by pulsing E high then low. */
static void pulse_enable(uint8_t value)
{
    expander_write(value | PIN_EN);
    delay_us(1);              /* E must be high for at least 450 ns */
    expander_write((uint8_t)(value & (uint8_t)~PIN_EN));
    delay_us(50);             /* most instructions complete in ~37 us */
}

static void write4(uint8_t nibble, uint8_t mode)
{
    pulse_enable((uint8_t)((nibble & 0xF0U) | mode));
}

static void write8(uint8_t byte, uint8_t mode)
{
    write4((uint8_t)(byte & 0xF0U), mode);
    write4((uint8_t)(byte << 4),    mode);
}

static void command(uint8_t c) { write8(c, 0); }
static void data(uint8_t c)    { write8(c, PIN_RS); }

i2c_status_t lcd_init(uint8_t addr7)
{
    s_addr = addr7;

    i2c_status_t st = i2c1_probe(addr7);
    if (st != I2C_OK) {
        return st;    /* nothing on that address - report instead of hanging */
    }

    delay_ms(50);                 /* datasheet wants >40 ms after power rises */

    /* Three 0x30 writes force a known 8-bit state whatever mode it woke up in. */
    write4(0x30, 0); delay_ms(5);
    write4(0x30, 0); delay_us(150);
    write4(0x30, 0); delay_us(150);

    write4(0x20, 0); delay_us(150);   /* now switch to 4-bit */

    command(0x28);   /* function set: 4-bit, 2 lines, 5x8 font */
    command(0x08);   /* display off while we configure          */
    command(0x01);   /* clear                                   */
    delay_ms(2);     /* clear is the slow one - 1.52 ms         */
    command(0x06);   /* entry mode: increment, no shift         */
    command(0x0C);   /* display on, cursor off, blink off       */

    return I2C_OK;
}

void lcd_clear(void)
{
    command(0x01);
    delay_ms(2);
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    static const uint8_t row_base[2] = { 0x00, 0x40 };   /* DDRAM layout, 16x2 */
    command((uint8_t)(0x80U | (row_base[row & 1U] + col)));
}

void lcd_print(const char *s)
{
    while (*s) {
        data((uint8_t)*s++);
    }
}

void lcd_backlight(int on)
{
    s_backlight = on ? PIN_BL : 0U;
    expander_write(0);
}
