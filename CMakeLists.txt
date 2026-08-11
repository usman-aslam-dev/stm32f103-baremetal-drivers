cmake_minimum_required(VERSION 3.16)
project(stm32f103_baremetal C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(ELF ${PROJECT_NAME}.elf)

add_executable(${ELF}
    src/startup_stm32f103rb.c
    src/clock.c
    src/gpio.c
    src/uart.c
    src/i2c.c
    src/adc.c
    src/systick.c
    src/dwt_delay.c
    src/lcd_pcf8574.c
    src/hcsr04.c
    src/main.c
)

target_include_directories(${ELF} PRIVATE include)

target_compile_options(${ELF} PRIVATE
    -Og -g3
    -Wall -Wextra -Wshadow -Wconversion
    -ffreestanding
)

target_link_options(${ELF} PRIVATE
    -T${CMAKE_SOURCE_DIR}/linker/stm32f103rb.ld
    -nostdlib
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
)

# Produce .bin and .hex, and print the flash/RAM footprint after every build -
# that number belongs in the README, so it should be impossible to miss.
add_custom_command(TARGET ${ELF} POST_BUILD
    COMMAND ${ARM_SIZE} $<TARGET_FILE:${ELF}>
    COMMAND ${ARM_OBJCOPY} -O binary $<TARGET_FILE:${ELF}> ${PROJECT_NAME}.bin
    COMMAND ${ARM_OBJCOPY} -O ihex   $<TARGET_FILE:${ELF}> ${PROJECT_NAME}.hex
    COMMENT "Size, .bin and .hex"
)
