/**
 * @file  startup_stm32f103rb.c
 * @brief Reset vector, C runtime bring-up and the interrupt vector table.
 *
 * This replaces the assembler startup file ST ships. Everything the CPU needs
 * between "power on" and "main() runs" happens here:
 *
 *   1. The vector table sits at 0x08000000 - the first word is the initial stack
 *      pointer, the second is the address of Reset_Handler. The hardware loads
 *      both automatically; nothing "calls" Reset_Handler.
 *   2. .data holds initialised globals. They live in flash but must be usable in
 *      RAM, so Reset_Handler copies them across.
 *   3. .bss holds zero-initialised globals. The C standard promises they are
 *      zero, and that promise is kept right here, by hand.
 *
 * Skip either step and you get variables containing whatever the last power
 * cycle left behind - a bug that looks random and is anything but.
 */
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;

int  main(void);
void Reset_Handler(void);
void Default_Handler(void);

/* Every unimplemented vector aliases to Default_Handler. Declaring them weak
 * means a driver can define e.g. SysTick_Handler and silently take over. */
#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))

void NMI_Handler(void)             WEAK_ALIAS;
void HardFault_Handler(void)       WEAK_ALIAS;
void MemManage_Handler(void)       WEAK_ALIAS;
void BusFault_Handler(void)        WEAK_ALIAS;
void UsageFault_Handler(void)      WEAK_ALIAS;
void SVC_Handler(void)             WEAK_ALIAS;
void DebugMon_Handler(void)        WEAK_ALIAS;
void PendSV_Handler(void)          WEAK_ALIAS;
void SysTick_Handler(void)         WEAK_ALIAS;
void USART2_IRQHandler(void)       WEAK_ALIAS;
void I2C1_EV_IRQHandler(void)      WEAK_ALIAS;
void I2C1_ER_IRQHandler(void)      WEAK_ALIAS;
void TIM3_IRQHandler(void)         WEAK_ALIAS;
void USB_HP_CAN1_TX_IRQHandler(void)  WEAK_ALIAS;
void USB_LP_CAN1_RX0_IRQHandler(void) WEAK_ALIAS;
void CAN1_RX1_IRQHandler(void)     WEAK_ALIAS;
void CAN1_SCE_IRQHandler(void)     WEAK_ALIAS;

__attribute__((section(".isr_vector"), used))
void (*const g_vectors[])(void) = {
    (void (*)(void))(&_estack),   /* 0x00: initial MSP  */
    Reset_Handler,                /* 0x04: reset        */
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,                   /* reserved */
    SVC_Handler,
    DebugMon_Handler,
    0,                            /* reserved */
    PendSV_Handler,
    SysTick_Handler,
    /* --- external interrupts, position 0 onward --- */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,      /*  0..9  */
    0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 10..18 */
    USB_HP_CAN1_TX_IRQHandler,         /* 19 */
    USB_LP_CAN1_RX0_IRQHandler,        /* 20 */
    CAN1_RX1_IRQHandler,               /* 21 */
    CAN1_SCE_IRQHandler,               /* 22 */
    0,                                 /* 23 EXTI9_5   */
    0,                                 /* 24 TIM1_BRK  */
    0, 0, 0,                           /* 25..27       */
    0,                                 /* 28 TIM2      */
    TIM3_IRQHandler,                   /* 29 TIM3      */
    0,                                 /* 30 TIM4      */
    I2C1_EV_IRQHandler,                /* 31 */
    I2C1_ER_IRQHandler,                /* 32 */
    0, 0, 0, 0,                        /* 33..36 */
    0,                                 /* 37 USART1 */
    USART2_IRQHandler,                 /* 38 */
};

void Reset_Handler(void)
{
    /* .data: copy initialised globals from their flash image into RAM. */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* .bss: zero it, because the C standard says these start at zero. */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0U;
    }

    (void)main();

    /* main() returning on an embedded target is a bug, not an exit. Park here
     * so a debugger can still attach and see where we ended up. */
    for (;;) { }
}

void Default_Handler(void)
{
    /* A breakpoint-friendly trap: if execution stops here, read the IPSR to see
     * which vector fired. */
    for (;;) { }
}
