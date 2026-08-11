/* Timing primitives are meaningless on the host - make them no-ops so the
 * driver logic under test runs instantly instead of busy-waiting. */
#include <stdint.h>
void     stub_systick(uint32_t hz)  { (void)hz; }
void     stub_delay_us(uint32_t us) { (void)us; }
void     stub_delay_ms(uint32_t ms) { (void)ms; }
uint32_t stub_millis(void)          { static uint32_t t; return t += 1000; }
