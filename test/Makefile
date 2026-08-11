# Host test build. Deliberately plain: no board, no cross-compiler, no CMake.
#   make -C test run
CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -DUNIT_TEST -I../include -I. -g -O0
SRC     := test_drivers.c support/fake_regs.c \
           ../src/gpio.c ../src/uart.c ../src/i2c.c ../src/adc.c ../src/hcsr04.c
# Stub out the timing helpers the drivers call but which mean nothing on x86.
CFLAGS  += -Dsystick_init_1khz=stub_systick -Ddelay_us=stub_delay_us \
           -Ddelay_ms=stub_delay_ms -Dmillis=stub_millis

all: run

runner: $(SRC) support/stubs.c
	$(CC) $(CFLAGS) $(SRC) support/stubs.c -o runner

run: runner
	./runner

clean:
	rm -f runner
.PHONY: all run clean
