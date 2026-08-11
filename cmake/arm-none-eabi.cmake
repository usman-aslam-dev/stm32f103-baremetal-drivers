# Toolchain file for bare-metal Cortex-M3.
#
# CMAKE_TRY_COMPILE_TARGET_TYPE is the important line: without it CMake tries to
# link a full executable to test the compiler, which fails on a freestanding
# target that has no _start or syscalls yet.

set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_CC  arm-none-eabi-gcc     REQUIRED)
find_program(ARM_CXX arm-none-eabi-g++)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(ARM_SIZE    arm-none-eabi-size    REQUIRED)

set(CMAKE_C_COMPILER   ${ARM_CC})
set(CMAKE_ASM_COMPILER ${ARM_CC})

set(MCU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")

set(CMAKE_C_FLAGS_INIT "${MCU_FLAGS} -ffunction-sections -fdata-sections -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${MCU_FLAGS} -Wl,--gc-sections -nostartfiles")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
