/**
 * @file  main.c
 * @brief Sensor node: scan the I2C bus, then stream sensor telemetry as CSV.
 *
 * Output on the ST-LINK virtual COM port at 115200 8N1, one line per second:
 *
 *     ms,temp_mdeg,vdda_mv,dist_mm
 *     1000,31250,3287,142
 *
 * CSV rather than pretty text on purpose - the Raspberry Pi gateway in the
 * companion project parses this directly, and it drops straight into a
 * spreadsheet or a plot without another tool in between.
 */
#include "clock.h"
#include "systick.h"
#include "dwt_delay.h"
#include "gpio.h"
#include "uart.h"
#include "i2c.h"
#include "adc.h"
#include "lcd_pcf8574.h"
#include "hcsr04.h"
#include "stm32f103.h"

#define LED_PORT   GPIOA
#define LED_PIN    5        /* LD2, the green user LED on the Nucleo */

/* Tiny integer-to-string helpers. Pulling in newlib's printf would cost several
 * KB of flash and a heap for the sake of four numbers. */
static void put_u32(uint32_t v)
{
    char buf[11];
    int i = 0;
    if (v == 0) { uart2_write_byte('0'); return; }
    while (v && i < 10) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i--) uart2_write_byte((uint8_t)buf[i]);
}

static void put_i32(int32_t v)
{
    if (v < 0) { uart2_write_byte('-'); v = -v; }
    put_u32((uint32_t)v);
}

/** Walk every 7-bit address and report which ones answer. */
static void i2c_scan(void)
{
    uart2_write("I2C scan: ");
    int found = 0;
    for (uint8_t a = 0x08; a < 0x78; a++) {
        if (i2c1_probe(a) == I2C_OK) {
            uart2_write("0x");
            const char *hex = "0123456789ABCDEF";
            uart2_write_byte((uint8_t)hex[a >> 4]);
            uart2_write_byte((uint8_t)hex[a & 0xF]);
            uart2_write_byte(' ');
            found++;
        }
    }
    if (!found) uart2_write("(nothing responded)");
    uart2_write("\n");
}

int main(void)
{
    /* If the PLL will not lock we keep running on the 8 MHz HSI. Every timing
     * constant would then be wrong, so this is reported rather than hidden. */
    int clk = clock_init_72mhz();

    dwt_init();
    systick_init_1khz(HCLK_HZ);

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    gpio_configure(LED_PORT, LED_PIN, GPIO_CFG_OUT_PP_2MHZ);

    uart2_init(115200);
    uart2_write("\n=== STM32F103RB bare-metal sensor node ===\n");
    uart2_write(clk == 0 ? "clock: PLL 72 MHz\n" : "clock: PLL FAILED, running HSI\n");

    i2c1_init(PCLK1_HZ, 100000);
    i2c_scan();

    int have_lcd = (lcd_init(LCD_ADDR_DEFAULT) == I2C_OK);
    uart2_write(have_lcd ? "lcd: ready\n" : "lcd: not found (continuing headless)\n");

    adc1_init();
    hcsr04_init();

    uart2_write("ms,temp_mdeg,vdda_mv,dist_mm\n");

    uint32_t next = millis();

    for (;;) {
        if ((int32_t)(millis() - next) < 0) {
            continue;
        }
        next += 1000U;

        gpio_write(LED_PORT, LED_PIN, 1);

        uint32_t vdda_mv  = adc_calc_vdda_mv(adc1_read_vrefint());
        int32_t  temp_mdeg = adc_calc_temp_mdeg(adc1_read_temp_raw(), vdda_mv);
        uint32_t echo_us  = hcsr04_read_us();
        uint32_t dist_mm  = hcsr04_us_to_mm(echo_us);

        put_u32(millis());        uart2_write_byte(',');
        put_i32(temp_mdeg);       uart2_write_byte(',');
        put_u32(vdda_mv);         uart2_write_byte(',');
        if (dist_mm == HCSR04_NO_ECHO) uart2_write("-1");
        else                           put_u32(dist_mm);
        uart2_write("\n");

        if (have_lcd) {
            lcd_set_cursor(0, 0);
            lcd_print("T:");
            lcd_set_cursor(1, 0);
            lcd_print("D:");
        }

        gpio_write(LED_PORT, LED_PIN, 0);
    }
}
