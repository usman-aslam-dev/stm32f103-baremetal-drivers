/**
 * @file  i2c.c
 * @brief Blocking I2C1 master for STM32F103. PB6 = SCL, PB7 = SDA.
 *
 * The F1 I2C peripheral is notoriously fiddly. Three things trip people up and
 * all three are handled explicitly here:
 *
 *  1. ADDR is cleared by reading SR1 *then* SR2. Reading only one leaves the
 *     peripheral stalled. See clear_addr().
 *  2. The 1-byte and 2-byte receive cases need different ACK/STOP ordering than
 *     the general N>2 case (RM0008 "Master receiver" flowchart). Getting this
 *     wrong gives you an extra byte clocked out, or a missing STOP.
 *  3. A slave that was reset mid-transfer can hold SDA low forever. Nothing in
 *     the peripheral fixes that - you have to bit-bang 9 clocks on SCL as GPIO.
 *     See i2c1_bus_recover().
 *
 * Everything blocks with a bounded spin count, so a dead bus returns
 * I2C_ERR_TIMEOUT instead of hanging the system.
 */

#include "i2c.h"
#include "gpio.h"
#include "stm32f103.h"

/* Generous but finite. At 100 kHz one byte is ~90 us; this is milliseconds. */
#define I2C_SPIN_LIMIT   100000UL

/* --------------------------------------------------------------------------
 * Pure timing math - no hardware touched, so the host tests cover these.
 * -------------------------------------------------------------------------- */

/**
 * Standard mode: SCL high and low periods are equal, so
 *   CCR = PCLK1 / (2 * SCL)
 * At 36 MHz / 100 kHz this gives 180.
 */
uint32_t i2c_calc_ccr_sm(uint32_t pclk1_hz, uint32_t scl_hz)
{
    uint32_t ccr = pclk1_hz / (2U * scl_hz);
    return (ccr < 4U) ? 4U : ccr;   /* RM0008: minimum allowed value is 4 */
}

/**
 * Fast mode with DUTY = 0 gives a 1:2 low:high ratio, so
 *   CCR = PCLK1 / (3 * SCL)
 * At 36 MHz / 400 kHz this gives 30.
 */
uint32_t i2c_calc_ccr_fm(uint32_t pclk1_hz, uint32_t scl_hz)
{
    uint32_t ccr = pclk1_hz / (3U * scl_hz);
    return (ccr < 1U) ? 1U : ccr;
}

/**
 * TRISE = (max rise time / T_PCLK1) + 1. Standard mode allows 1000 ns, which is
 * exactly one PCLK1 period per MHz, so this collapses to FREQ + 1 = 37 at 36 MHz.
 */
uint32_t i2c_calc_trise_sm(uint32_t pclk1_hz)
{
    return (pclk1_hz / 1000000U) + 1U;
}

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

/** Spin until `mask` appears in SR1, or give up. Also aborts early on NACK. */
static i2c_status_t wait_sr1(uint32_t mask)
{
    uint32_t spins = I2C_SPIN_LIMIT;
    while (spins--) {
        uint32_t sr1 = I2C1->SR1;

        if (sr1 & I2C_SR1_AF) {              /* slave refused the address/byte */
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERR_NACK;
        }
        if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO)) {
            I2C1->SR1 &= ~(I2C_SR1_BERR | I2C_SR1_ARLO);
            return I2C_ERR_BUS;
        }
        if (sr1 & mask) {
            return I2C_OK;
        }
    }
    return I2C_ERR_TIMEOUT;
}

/**
 * The ADDR flag is cleared by a read of SR1 followed by a read of SR2.
 * The `(void)` casts stop the compiler optimising the reads away - this is
 * exactly why the register struct members are volatile.
 */
static void clear_addr(void)
{
    (void)I2C1->SR1;
    (void)I2C1->SR2;
}

static i2c_status_t start_and_address(uint8_t addr7, int reading)
{
    i2c_status_t st;

    I2C1->CR1 |= I2C_CR1_START;
    st = wait_sr1(I2C_SR1_SB);
    if (st != I2C_OK) return st;

    /* 7-bit address occupies bits 7:1, bit 0 is the direction flag. */
    I2C1->DR = (uint32_t)(addr7 << 1) | (reading ? 1U : 0U);

    return wait_sr1(I2C_SR1_ADDR);
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void i2c1_init(uint32_t pclk1_hz, uint32_t scl_hz)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* Both lines must be alternate-function OPEN DRAIN. Push-pull here is the
     * single most common I2C bring-up mistake - it fights the pull-ups and the
     * bus never goes anywhere. */
    gpio_configure(GPIOB, 6, GPIO_CFG_AF_OD_50MHZ);   /* SCL */
    gpio_configure(GPIOB, 7, GPIO_CFG_AF_OD_50MHZ);   /* SDA */

    /* Reset the peripheral before configuring - clears any wedged state. */
    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;

    I2C1->CR2 = (pclk1_hz / 1000000U) & I2C_CR2_FREQ_Msk;

    if (scl_hz > 100000U) {
        I2C1->CCR   = I2C_CCR_FS | i2c_calc_ccr_fm(pclk1_hz, scl_hz);
        I2C1->TRISE = ((pclk1_hz / 1000000U) * 300U) / 1000U + 1U;  /* 300 ns Fm */
    } else {
        I2C1->CCR   = i2c_calc_ccr_sm(pclk1_hz, scl_hz);
        I2C1->TRISE = i2c_calc_trise_sm(pclk1_hz);
    }

    I2C1->CR1 |= I2C_CR1_PE;
}

i2c_status_t i2c1_write(uint8_t addr7, const uint8_t *data, size_t n)
{
    i2c_status_t st = start_and_address(addr7, 0);
    if (st != I2C_OK) return st;

    clear_addr();

    for (size_t i = 0; i < n; i++) {
        st = wait_sr1(I2C_SR1_TXE);
        if (st != I2C_OK) return st;
        I2C1->DR = data[i];
    }

    /* BTF means the shift register has drained too - only then is STOP safe.
     * Sending STOP on TXE alone can truncate the final byte. */
    st = wait_sr1(I2C_SR1_BTF);
    if (st != I2C_OK) return st;

    I2C1->CR1 |= I2C_CR1_STOP;
    return I2C_OK;
}

/**
 * Master receive. Three distinct paths, straight out of the RM0008 flowchart.
 * This is the function to be able to explain line by line in an interview.
 */
i2c_status_t i2c1_read(uint8_t addr7, uint8_t *data, size_t n)
{
    i2c_status_t st;

    if (n == 0) return I2C_OK;

    I2C1->CR1 |= I2C_CR1_ACK;

    if (n == 1) {
        /* ---- Single byte: ACK off and STOP armed *before* ADDR is cleared,
         * otherwise the peripheral starts clocking a second byte we do not
         * want and cannot stop in time. ---- */
        st = start_and_address(addr7, 1);
        if (st != I2C_OK) return st;

        I2C1->CR1 &= ~I2C_CR1_ACK;
        clear_addr();
        I2C1->CR1 |= I2C_CR1_STOP;

        st = wait_sr1(I2C_SR1_RXNE);
        if (st != I2C_OK) return st;
        data[0] = (uint8_t)I2C1->DR;

    } else if (n == 2) {
        /* ---- Two bytes: POS shifts the NACK one position so it lands on the
         * second byte. Both bytes are then lifted out together on BTF. ---- */
        I2C1->CR1 |= I2C_CR1_POS;

        st = start_and_address(addr7, 1);
        if (st != I2C_OK) return st;

        clear_addr();
        I2C1->CR1 &= ~I2C_CR1_ACK;

        st = wait_sr1(I2C_SR1_BTF);
        if (st != I2C_OK) return st;

        I2C1->CR1 |= I2C_CR1_STOP;
        data[0] = (uint8_t)I2C1->DR;
        data[1] = (uint8_t)I2C1->DR;

        I2C1->CR1 &= ~I2C_CR1_POS;   /* leave the peripheral as we found it */

    } else {
        /* ---- N > 2: stream on RXNE until three remain, then the documented
         * tail sequence that guarantees a NACK on the last byte only. ---- */
        st = start_and_address(addr7, 1);
        if (st != I2C_OK) return st;

        clear_addr();

        size_t i = 0;
        while (n - i > 3U) {
            st = wait_sr1(I2C_SR1_RXNE);
            if (st != I2C_OK) return st;
            data[i++] = (uint8_t)I2C1->DR;
        }

        /* three left: N-3, N-2, N-1 */
        st = wait_sr1(I2C_SR1_BTF);
        if (st != I2C_OK) return st;
        I2C1->CR1 &= ~I2C_CR1_ACK;
        data[i++] = (uint8_t)I2C1->DR;         /* N-3 */

        st = wait_sr1(I2C_SR1_BTF);
        if (st != I2C_OK) return st;
        I2C1->CR1 |= I2C_CR1_STOP;
        data[i++] = (uint8_t)I2C1->DR;         /* N-2 */
        data[i++] = (uint8_t)I2C1->DR;         /* N-1 */
    }

    return I2C_OK;
}

/** Address-only transaction. Used by the bus scanner in main.c. */
i2c_status_t i2c1_probe(uint8_t addr7)
{
    i2c_status_t st = start_and_address(addr7, 0);
    if (st == I2C_OK) {
        clear_addr();
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    return st;
}

/**
 * Recovery for a slave that is holding SDA low because it was reset in the
 * middle of a byte. The peripheral cannot fix this; the lines are temporarily
 * reclaimed as GPIO and clocked manually until the slave finishes its byte and
 * releases SDA, then a STOP condition is generated by hand.
 */
void i2c1_bus_recover(void)
{
    I2C1->CR1 &= ~I2C_CR1_PE;

    gpio_configure(GPIOB, 6, GPIO_CFG_OUT_OD_50MHZ);   /* SCL */
    gpio_configure(GPIOB, 7, GPIO_CFG_OUT_OD_50MHZ);   /* SDA */
    gpio_set(GPIOB, 7);

    for (int i = 0; i < 9; i++) {
        gpio_clear(GPIOB, 6);
        for (volatile int d = 0; d < 200; d++) { }
        gpio_set(GPIOB, 6);
        for (volatile int d = 0; d < 200; d++) { }
        if (gpio_read(GPIOB, 7)) {
            break;   /* slave released SDA */
        }
    }

    /* Manual STOP: SDA low -> high while SCL is high. */
    gpio_clear(GPIOB, 7);
    for (volatile int d = 0; d < 200; d++) { }
    gpio_set(GPIOB, 6);
    for (volatile int d = 0; d < 200; d++) { }
    gpio_set(GPIOB, 7);

    gpio_configure(GPIOB, 6, GPIO_CFG_AF_OD_50MHZ);
    gpio_configure(GPIOB, 7, GPIO_CFG_AF_OD_50MHZ);

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;
    I2C1->CR1 |= I2C_CR1_PE;
}
