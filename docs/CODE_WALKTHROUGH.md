# Code Walkthrough — every file, what it does, and how to defend it

This is the document to read the night before an interview. One section per
file: what it is for, the two or three decisions inside it that a reviewer will
question, and the answer.

---

## `include/stm32f103.h` — the register map

**What it is.** Every peripheral this project touches, described as a C struct
whose members land exactly on the hardware register offsets, plus named
constants for the bit fields used.

**The core trick.** A peripheral is a block of registers at a fixed address.
Describe that block as a `volatile` struct and point it at the base address:

```c
typedef struct {
    volatile uint32_t CR1, CR2, OAR1, OAR2, DR, SR1, SR2, CCR, TRISE;
} I2C_TypeDef;

#define I2C1  ((I2C_TypeDef *)0x40005400UL)
```

Now `I2C1->CR1 |= I2C_CR1_START;` compiles to a load, an OR and a store at
`0x40005400`. No library, no abstraction.

**Why `volatile` is not optional.** Without it the compiler is entitled to
assume nothing changes a variable behind its back, so:

```c
while (!(I2C1->SR1 & I2C_SR1_SB)) { }
```

gets optimised into "read `SR1` once, and if the bit is clear loop forever."
`volatile` forces a real memory access on every iteration. **This is the single
most common bare-metal bug and a guaranteed interview question.**

**Why hand-written instead of CMSIS.** CMSIS headers are correct and I would
use them at work. Writing this one by hand is the entire pedagogical point:
it proves the offsets came from RM0008 rather than from an include I never
opened. Say exactly that — do not pretend hand-rolling is better engineering.

**Likely question:** *"Why did you not just include `stm32f1xx.h`?"*
→ Because the exercise was to demonstrate I can read a reference manual. In
production I would use the vendor headers; reinventing them is a maintenance
liability, not a virtue.

---

## `src/startup_stm32f103rb.c` — what runs before `main`

**What it is.** The reset vector and the C runtime bring-up that normally
arrives invisibly from the vendor's assembly file.

**What happens at reset.** The Cortex-M3 does something unusual: it loads the
initial stack pointer from address `0x00000000` and the reset address from
`0x00000004`, *in hardware*, before executing a single instruction. That is why
the first entry in the vector table is not a function pointer at all — it is
`&_estack`, cast to look like one.

**The three jobs of `Reset_Handler`:**

1. **Copy `.data`.** Initialised globals (`int x = 5;`) have their values
   stored in flash but must live in RAM. The loop copies from the load address
   (`_sidata`) to the run address (`_sdata`.. `_edata`). Skip this and every
   initialised global holds garbage.
2. **Zero `.bss`.** C guarantees uninitialised statics start at zero. Nothing
   else does this on bare metal — SRAM at power-on contains whatever it
   contains.
3. **Call `main`.**

**Why the loop after `main`.** `main` must never return on an embedded target;
there is nothing to return *to*. The infinite loop makes that explicit rather
than letting the PC run into whatever bytes follow.

**Likely question:** *"What is the first thing the CPU does after reset?"*
→ Fetches MSP from `0x0`, then PC from `0x4`. Both in hardware, before any code.

---

## `linker/stm32f103rb.ld` — where everything lives

**What it is.** The memory map: 128 KB flash at `0x08000000`, 20 KB SRAM at
`0x20000000`, and rules assigning each section to one of them.

**The part that matters — LMA vs VMA.** `.data` has two addresses: it is
*stored* in flash (load address) and *runs* in RAM (virtual address). The
`AT >FLASH` directive creates that split, and the `_sidata` symbol tells
`Reset_Handler` where to copy from. Understanding this distinction is the
difference between having used a linker script and having written one.

**Stack placement.** `_estack = ORIGIN(RAM) + LENGTH(RAM)` puts the stack at
the very top of SRAM, growing downwards toward `.bss`. On Cortex-M there is no
MPU guard by default, so an overflow silently corrupts `.bss` rather than
faulting. The `._user_heap_stack` section exists purely to make the build
*fail* if the requested stack cannot fit — a link error beats a runtime
mystery.

**Likely question:** *"How does an initialised global get its value?"*
→ Stored in flash at the LMA, copied to RAM by the startup code using symbols
the linker script defines.

---

## `src/clock.c` — 8 MHz in, 72 MHz out

**What it is.** The PLL bring-up. Roughly forty lines that everything else in
the system depends on.

**The order is the content.** Get any step out of sequence and the chip either
hangs or faults:

1. `HSEBYP` **then** `HSEON` — bypass first, because we are being fed a square
   wave from the ST-LINK's MCO, not driving a crystal. Set `HSEON` without
   `HSEBYP` on this board and `HSERDY` never asserts.
2. **Flash wait states before raising the clock.** `LATENCY=2` is required above
   48 MHz. Program it while still slow. Raise the clock first and the core
   out-runs the flash → HardFault before `main`.
3. **Bus prescalers before selecting the PLL.** APB1 has a hard 36 MHz ceiling,
   so `PPRE1` must already be `/2` at the instant SYSCLK becomes 72 MHz.
   Set them afterwards and APB1 is briefly overclocked at 72 MHz.
4. PLL: source HSE, `×9` → 72 MHz. Enable, wait for `PLLRDY`.
5. Switch `SW` to PLL, then **poll `SWS` until it reads back**. Requesting the
   switch and assuming it happened is a real bug — the hardware takes time.

**ADC prescaler.** ADCCLK max is 14 MHz. From 72 MHz the options are /2, /4, /6,
/8 → 36, 18, **12**, 9. Only /6 and /8 are legal; /6 is chosen. A reviewer may
well ask why not /4 — because 18 MHz exceeds the datasheet limit.

**Why it returns a status code.** If the PLL never locks, every timing constant
downstream is wrong by a factor of nine. `main` checks the return and stops
visibly rather than running a system that lies about time.

**Likely question:** *"Why 2 flash wait states?"* → SYSCLK 72 MHz is above the
48 MHz threshold in RM0008. *"Why HSEBYP?"* → square wave from ST-LINK MCO, not
a crystal.

---

## `src/gpio.c` — the F1 CRL/CRH model

**What it is.** Pin configuration for the STM32F1 family, which is genuinely
different from every later STM32.

**The F1 is the odd one out.** F4/L4/G4 use four registers
(`MODER`/`OTYPER`/`OSPEEDR`/`PUPDR`). The F1 packs everything into **4 bits per
pin**: `CNF[1:0]` in the upper two, `MODE[1:0]` in the lower two. Sixteen pins
× 4 bits = 64 bits, which does not fit in one 32-bit register — hence two:
`CRL` for pins 0–7, `CRH` for pins 8–15.

```
pin < 8  →  CRL, shift = pin * 4
pin >= 8 →  CRH, shift = (pin - 8) * 4
```

`MODE` doubles as both direction and slew rate: `00` = input, `01/10/11` =
output at 10/2/50 MHz. `CNF`'s meaning flips depending on whether `MODE` says
input or output — which is why the nibble table in `stm32f103.h` exists rather
than composing bits at each call site.

**Input pull-up vs pull-down.** Both are `CNF=10, MODE=00`. The *direction* is
selected by writing `ODR` — 1 for pull-up, 0 for pull-down. That is the F1's
strangest quirk and a good depth-check question.

**BSRR, always.** `gpio_set`/`gpio_clear` write `BSRR`, a write-only register
where the low 16 bits set and the high 16 bits reset. One atomic store. The
alternative, `ODR |= (1<<pin)`, is read-modify-write: three instructions, and
an interrupt landing in the middle that touches another pin on the same port
loses that write.

**Likely question:** *"How is F1 GPIO different from F4?"* → CRL/CRH with
MODE+CNF nibbles versus the four separate registers.

---

## `src/uart.c` — USART2 and the ST-LINK console

**What it is.** 115200 8N1 on PA2/PA3, which the Nucleo routes to the ST-LINK's
virtual COM port through solder bridges SB13/SB14. Free console, no FTDI
adapter, no extra wiring.

**The baud rate divisor.** `USART_BRR` is fixed-point: bits [15:4] are the
integer mantissa, bits [3:0] the fraction in sixteenths.

```
USARTDIV = PCLK1 / (16 × baud)
At 36 MHz, 115200:  36e6 / (16 × 115200) = 19.53
  mantissa = 19,  fraction = 0.53 × 16 ≈ 8
  BRR = (19 << 4) | 8 = 0x138
```

`uart_calc_brr()` is a pure function precisely so this arithmetic is unit-tested
on the host instead of being verified by squinting at garbled terminal output.

**TXE vs TC.** `TXE` means the data register is free for the *next* byte; `TC`
means the last byte has fully left the shift register. Poll `TXE` between bytes
for throughput; wait for `TC` before disabling the peripheral or dropping a
transceiver's direction line, or you truncate the final character.

**Clearing ORE.** Overrun sets `ORE`, and until it is cleared `RXNE` stops
being useful — the receiver is wedged. The clear sequence is *read `SR`, then
read `DR`*. Not one or the other. A receiver that goes deaf after a burst is
almost always a missing ORE clear.

**Likely question:** *"Your UART receives fine, then stops after a burst. Why?"*
→ Overrun; `ORE` set and never cleared.

---

## `src/i2c.c` — the hard one

**What it is.** A blocking I²C1 master. The F1's I²C peripheral has a
reputation, and this file is where it is earned.

**Pins must be alternate-function OPEN-DRAIN** (nibble `0xF`). I²C is a
wired-AND bus; push-pull would have master and slave fighting over the line
and could damage both. Configuring push-pull here is a classic and expensive
mistake.

**Timing setup.**
- `CR2.FREQ` = APB1 in MHz (36) — tells the peripheral its own input clock.
- `CCR` = `PCLK1 / (2 × SCL)` = `36e6 / 200e3` = **180** for 100 kHz.
- `TRISE` = `FREQ + 1` = **37** (1000 ns max rise time in standard mode).

All three are pure functions, host-tested.

**Clearing ADDR — read `SR1`, then `SR2`.** After the address is ACKed, `ADDR`
sets and **SCL is held low** until it is cleared. The clear sequence is a read
of `SR1` followed by a read of `SR2`. Read only one and the bus stays stretched
forever, which presents as a total hang. There is a helper (`clear_addr()`)
purely so this cannot be got wrong at one call site out of five.

**Why N=1 and N=2 receives are special.** NACK on the final byte must be set up
*before* that byte finishes arriving, and with only one or two bytes there is
no room to react afterwards:
- **N=1:** clear `ACK` and set `STOP` *while `ADDR` is being cleared*, then read.
- **N=2:** set `POS` before addressing, clear `ACK` after `ADDR`, wait for
  `BTF`, set `STOP`, then read both bytes back-to-back.
- **N>2:** the straightforward loop, NACK before the last byte.

These come straight from RM0008's master-receiver flowchart. Using the general
path for N=1 clocks an extra byte out of the slave — which, on a sensor with
auto-incrementing registers, silently corrupts the next read too.

**Bus recovery.** If a slave was reset mid-transfer it can hold SDA low
forever, and no amount of peripheral reset fixes that — the *slave* needs
clocks to finish its transaction. `i2c1_bus_recover()` reconfigures SCL as a
GPIO, toggles it 9 times, then asserts `CR1.SWRST`. Nine, because that is one
byte plus the ACK.

**Likely question:** *"Your I²C hangs on the first transfer. Where do you look?"*
→ ADDR not cleared (SR1 then SR2), or the pins configured push-pull instead of
open-drain.

---

## `src/adc.c` — internal temperature and true VDDA

**What it is.** ADC1 driving two internal channels: the temperature sensor
(IN16) and the bandgap reference VREFINT (IN17).

**The chicken-and-egg problem this solves.** The ADC measures ratiometrically
against VDDA. Assume VDDA = 3.3 V and every reading inherits the supply's
error — USB-powered boards routinely sit at 3.2 V. But VREFINT is a known
1.20 V, so measuring *it* reveals the actual supply:

```
VDDA_mV = 1200 × 4095 / vrefint_raw
```

Then any other channel converts with real millivolts. This is the trick worth
knowing and it generalises to every STM32.

**Sampling time is not optional.** The temperature sensor needs **17.1 µs**
minimum. At 12 MHz ADCCLK the 239.5-cycle setting gives ~20 µs. Leave the
default short sampling time and readings are plausible-looking nonsense —
the sample capacitor never charges.

**Calibration.** `RSTCAL` then `CAL`, polling each until it self-clears.
Removes internal offset error. `ADON` must be written **twice**: the first
write wakes the ADC from power-down (t_STAB), the second starts conversion.

**Temperature formula:** `T = (V25 − Vsense)/Avg_Slope + 25`, V25 = 1.43 V,
Avg_Slope = 4.3 mV/°C. Integer arithmetic throughout — no FPU on a Cortex-M3,
so floats mean a software library and a large flash cost.

**Be honest about accuracy.** ±45 °C absolute, uncalibrated. Volunteer this
before being asked; it demonstrates you read the datasheet's tolerance column
rather than just its typical column. The sensor is genuinely useful for
*relative* change, which is what the FreeRTOS project uses it for.

**Likely question:** *"How accurate is that reading?"* → Poor in absolute terms
(±45 °C), good for trends; VREFINT removes supply error but not sensor spread.

---

## `src/lcd_pcf8574.c` — two protocols stacked

**What it is.** A 16×2 HD44780 character LCD driven through a PCF8574 I²C I/O
expander. Two protocols in series: I²C to reach the expander, then HD44780's
parallel interface emulated through the expander's 8 pins.

**Pin mapping** (the near-universal backpack wiring): P0=RS, P1=RW, P2=E,
P3=backlight, P4–P7=D4–D7.

**Every LCD nibble costs three I²C writes.** The HD44780 latches data on the
falling edge of E, so each nibble is: set data with E low, set E high, set E
low. One byte = two nibbles = six I²C transactions. That is why this display is
slow, and why the code batches where it can.

**The power-on ritual is not superstition.** Wait 40 ms, write `0x3` three
times (with 4.1 ms then 100 µs gaps), then `0x2` to switch to 4-bit mode. The
controller boots in 8-bit mode and this specific sequence is the documented way
to get it into 4-bit mode from an unknown state — including from a state where
it is *already* in 4-bit mode after a warm reset.

**Two instructions are slow.** Clear and Home take **1.52 ms**; everything else
~37 µs. Treat them all as 37 µs and the display fills with garbage.

**Likely question:** *"Why three writes per nibble?"* → E must be pulsed
high-then-low to latch, and every pin change goes through a separate I²C
transaction to the expander.

---

## `src/hcsr04.c` — timer input capture

**What it is.** Ultrasonic distance measurement via timer input capture, which
is the correct technique for measuring a pulse width without burning CPU.

**Prescaler.** `PSC = 71` divides 72 MHz to exactly **1 MHz** → one tick per
microsecond. Now the captured count *is* the pulse width in µs, no arithmetic.

**Why capture and not polling.** Polling in a loop while timing with SysTick
gives you jitter equal to your loop period and stops working the moment an
interrupt lands. Input capture latches the counter value **in hardware** at the
edge — the measurement is exact regardless of what software is doing.

**Distance.** `distance_cm = pulse_µs / 58`. Sound travels ~343 m/s at 20 °C,
the pulse covers the distance twice, so 2 cm / 343 m·s⁻¹ ≈ 58 µs per cm.

**Temperature dependence.** Speed of sound changes ~0.6 m/s per °C, so a
reading calibrated at 20 °C drifts ~0.17 % per °C. Over 0–40 °C that is about
±3 % — worth mentioning unprompted, and a natural hook to the temperature
sensor in the same project.

**Timeout.** No echo (nothing in range) means the pulse never ends. Max range
~4 m ≈ 23 ms round trip, so the timer's update event bounds the wait and the
function returns an error instead of hanging.

**Likely question:** *"Why input capture rather than polling?"* → Hardware
latches the count at the edge; immune to interrupt jitter and software timing.

---

## `src/systick.c` and `src/dwt_delay.c` — two clocks for two jobs

**SysTick** is a 24-bit down-counter in the core. `LOAD = 71999` at 72 MHz
gives a 1 ms tick driving a millisecond counter for coarse delays and timeouts.

**DWT_CYCCNT** is a free-running 32-bit core-cycle counter in the debug unit.
Enable via `DEMCR.TRCENA` then `DWT_CTRL.CYCCNTENA`. At 72 MHz one tick is
13.9 ns, which is what the HD44780's ~40 µs pulses and the HC-SR04's 10 µs
trigger actually need. SysTick's 1 ms resolution is useless at that scale.

It is also the measurement tool: bracket any code with two `CYCCNT` reads and
you have an exact cycle count. On a board with no oscilloscope this is the only
honest way to report execution time — and it is how the WCET numbers in the
FreeRTOS project were obtained.

**Note:** `CYCCNT` wraps every ~59.6 s at 72 MHz. Unsigned subtraction handles
the wrap correctly for intervals shorter than that; longer intervals need
SysTick.

**Likely question:** *"How would you measure how long a function takes with no
scope?"* → DWT cycle counter, difference of two reads.

---

## `test/` — proving drivers on x86

**The technique.** Peripheral pointers are macros. Under `-DUNIT_TEST` they are
redirected from real addresses to a RAM struct:

```c
#ifdef UNIT_TEST
  extern I2C_TypeDef fake_i2c1;
  #define I2C1 (&fake_i2c1)
#else
  #define I2C1 ((I2C_TypeDef *)0x40005400UL)
#endif
```

Driver code compiles unchanged. The test sets up the fake status register,
calls the driver, and asserts what got written. Genuine unit testing of
register-level code with no hardware.

**What it covers:** BRR divisors, I²C CCR/TRISE arithmetic, ADC conversion
maths across the VDDA range, HC-SR04 distance conversion including the timeout
path, GPIO nibble placement in CRL vs CRH.

**What it cannot cover — say this before being asked:** it proves the driver
writes the values it *intends* to. It cannot prove the silicon responds as
documented. That still requires a board, and ultimately an oscilloscope for
anything analogue.

**Likely question:** *"How do you unit-test code that touches hardware?"*
→ Abstract the register access, substitute a fake bank on the host, assert on
the writes.

---

## Interview one-liners

| Question | Answer |
|---|---|
| Why `volatile` on registers? | Compiler would cache the read and loop forever on a status poll |
| Why HSEBYP on this board? | HSE is an 8 MHz square wave from ST-LINK MCO, not a crystal |
| Why flash wait states first? | Above 48 MHz the core out-runs flash → HardFault before `main` |
| F1 vs F4 GPIO? | CRL/CRH 4-bit MODE+CNF nibbles vs MODER/OTYPER/OSPEEDR/PUPDR |
| Why BSRR not ODR? | Atomic single store; ODR is read-modify-write and interrupt-unsafe |
| How to clear I²C ADDR? | Read SR1 then SR2 — SCL is stretched until you do |
| Why is I²C N=1 special? | NACK must be armed before the byte completes; no time to react after |
| I²C bus stuck low? | Toggle SCL 9 times as GPIO, then CR1.SWRST |
| UART deaf after a burst? | ORE set; clear by reading SR then DR |
| Temp sensor accuracy? | ±45 °C absolute uncalibrated; use VREFINT for true VDDA |
| Timing without a scope? | DWT_CYCCNT delta |
