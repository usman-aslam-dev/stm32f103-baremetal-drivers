/**
 * @file    stm32f103.h
 * @brief   Minimal register map for STM32F103RB (Cortex-M3, 128K flash / 20K SRAM).
 *
 * Written by hand from RM0008 rather than pulled from CMSIS on purpose: the point
 * of this project is to show the peripheral layout is understood, not copied.
 *
 * Every peripheral is a packed struct of `volatile uint32_t` laid over the peripheral
 * base address. Reserved gaps are named explicitly so the offsets self-document and
 * a wrong offset shows up as a compile-time struct-size assert (see STATIC_ASSERT
 * blocks at the bottom) instead of a silent runtime fault.
 */
#ifndef STM32F103_H
#define STM32F103_H

#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Test seam                                                                  */
/* ------------------------------------------------------------------------- */
/*
 * On the target, peripheral pointers resolve to real addresses.
 * Under host unit tests (-DUNIT_TEST) they resolve to RAM structs, so the exact
 * same driver source can be exercised on x86 and the register writes inspected.
 * This is the "fake register bank" technique.
 */
#ifdef UNIT_TEST
#define PERIPH_DECL extern
#else
#define PERIPH_DECL
#endif

/* ------------------------------------------------------------------------- */
/* Bus base addresses (RM0008 Table 3, Memory map)                            */
/* ------------------------------------------------------------------------- */
#define FLASH_BASE      0x08000000UL
#define SRAM_BASE       0x20000000UL
#define PERIPH_BASE     0x40000000UL

#define APB1_BASE       (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHB_BASE        (PERIPH_BASE + 0x00020000UL)

/* ------------------------------------------------------------------------- */
/* RCC - Reset and Clock Control                                              */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR;        /* 0x00 clock control                        */
    volatile uint32_t CFGR;      /* 0x04 clock configuration                  */
    volatile uint32_t CIR;       /* 0x08 clock interrupt                      */
    volatile uint32_t APB2RSTR;  /* 0x0C APB2 peripheral reset                */
    volatile uint32_t APB1RSTR;  /* 0x10 APB1 peripheral reset                */
    volatile uint32_t AHBENR;    /* 0x14 AHB peripheral clock enable          */
    volatile uint32_t APB2ENR;   /* 0x18 APB2 peripheral clock enable         */
    volatile uint32_t APB1ENR;   /* 0x1C APB1 peripheral clock enable         */
    volatile uint32_t BDCR;      /* 0x20 backup domain control                */
    volatile uint32_t CSR;       /* 0x24 control/status                       */
} RCC_TypeDef;

#define RCC_BASE        (AHB_BASE + 0x1000UL)   /* 0x40021000 */

/* RCC_CR */
#define RCC_CR_HSION        (1UL << 0)
#define RCC_CR_HSIRDY       (1UL << 1)
#define RCC_CR_HSEON        (1UL << 16)
#define RCC_CR_HSERDY       (1UL << 17)
#define RCC_CR_HSEBYP       (1UL << 18)  /* Nucleo: HSE is a square wave, not a crystal */
#define RCC_CR_CSSON        (1UL << 19)
#define RCC_CR_PLLON        (1UL << 24)
#define RCC_CR_PLLRDY       (1UL << 25)

/* RCC_CFGR */
#define RCC_CFGR_SW_Pos         0U
#define RCC_CFGR_SW_Msk         (3UL << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_HSI         (0UL << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_HSE         (1UL << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_PLL         (2UL << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SWS_Pos        2U
#define RCC_CFGR_SWS_Msk        (3UL << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_PLL        (2UL << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_HPRE_Pos       4U
#define RCC_CFGR_HPRE_DIV1      (0UL << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_PPRE1_Pos      8U
#define RCC_CFGR_PPRE1_DIV2     (4UL << RCC_CFGR_PPRE1_Pos)  /* APB1 max 36 MHz  */
#define RCC_CFGR_PPRE2_Pos      11U
#define RCC_CFGR_PPRE2_DIV1     (0UL << RCC_CFGR_PPRE2_Pos)  /* APB2 max 72 MHz  */
#define RCC_CFGR_ADCPRE_Pos     14U
#define RCC_CFGR_ADCPRE_DIV6    (2UL << RCC_CFGR_ADCPRE_Pos) /* 72/6 = 12 MHz    */
#define RCC_CFGR_PLLSRC         (1UL << 16)
#define RCC_CFGR_PLLXTPRE       (1UL << 17)
#define RCC_CFGR_PLLMULL_Pos    18U
#define RCC_CFGR_PLLMULL9       (7UL << RCC_CFGR_PLLMULL_Pos) /* 8 MHz x9 = 72   */

/* RCC_APB2ENR */
#define RCC_APB2ENR_AFIOEN      (1UL << 0)
#define RCC_APB2ENR_IOPAEN      (1UL << 2)
#define RCC_APB2ENR_IOPBEN      (1UL << 3)
#define RCC_APB2ENR_IOPCEN      (1UL << 4)
#define RCC_APB2ENR_IOPDEN      (1UL << 5)
#define RCC_APB2ENR_ADC1EN      (1UL << 9)
#define RCC_APB2ENR_ADC2EN      (1UL << 10)
#define RCC_APB2ENR_TIM1EN      (1UL << 11)
#define RCC_APB2ENR_USART1EN    (1UL << 14)

/* RCC_APB1ENR */
#define RCC_APB1ENR_TIM2EN      (1UL << 0)
#define RCC_APB1ENR_TIM3EN      (1UL << 1)
#define RCC_APB1ENR_TIM4EN      (1UL << 2)
#define RCC_APB1ENR_USART2EN    (1UL << 17)
#define RCC_APB1ENR_USART3EN    (1UL << 18)
#define RCC_APB1ENR_I2C1EN      (1UL << 21)
#define RCC_APB1ENR_I2C2EN      (1UL << 22)
#define RCC_APB1ENR_CAN1EN      (1UL << 25)
#define RCC_APB1ENR_PWREN       (1UL << 28)

/* ------------------------------------------------------------------------- */
/* FLASH interface (wait states / prefetch)                                   */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t ACR;       /* 0x00 access control */
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t AR;
    volatile uint32_t RESERVED;
    volatile uint32_t OBR;
    volatile uint32_t WRPR;
} FLASH_TypeDef;

#define FLASH_R_BASE    (AHB_BASE + 0x2000UL)   /* 0x40022000 */

#define FLASH_ACR_LATENCY_0     (0UL << 0)   /* SYSCLK <= 24 MHz */
#define FLASH_ACR_LATENCY_1     (1UL << 0)   /* 24  < SYSCLK <= 48 MHz */
#define FLASH_ACR_LATENCY_2     (2UL << 0)   /* 48  < SYSCLK <= 72 MHz */
#define FLASH_ACR_PRFTBE        (1UL << 4)

/* ------------------------------------------------------------------------- */
/* GPIO - the F1 CRL/CRH model (NOT the F4 MODER model)                       */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CRL;       /* 0x00 pins 0..7,  4 bits each */
    volatile uint32_t CRH;       /* 0x04 pins 8..15, 4 bits each */
    volatile uint32_t IDR;       /* 0x08 input data              */
    volatile uint32_t ODR;       /* 0x0C output data             */
    volatile uint32_t BSRR;      /* 0x10 atomic set(15:0)/reset(31:16) */
    volatile uint32_t BRR;       /* 0x14 atomic reset            */
    volatile uint32_t LCKR;      /* 0x18 configuration lock      */
} GPIO_TypeDef;

#define GPIOA_BASE      (APB2_BASE + 0x0800UL)  /* 0x40010800 */
#define GPIOB_BASE      (APB2_BASE + 0x0C00UL)
#define GPIOC_BASE      (APB2_BASE + 0x1000UL)
#define GPIOD_BASE      (APB2_BASE + 0x1400UL)

/*
 * 4-bit pin configuration nibbles: CNF[3:2] | MODE[1:0].
 * These are the values the whole F1 family lives on; get them wrong and the pin
 * silently does nothing, which is why they are named rather than magic numbers.
 */
#define GPIO_CFG_IN_ANALOG      0x0U   /* CNF=00 MODE=00 */
#define GPIO_CFG_IN_FLOATING    0x4U   /* CNF=01 MODE=00 - reset state */
#define GPIO_CFG_IN_PULL        0x8U   /* CNF=10 MODE=00 - direction from ODR */
#define GPIO_CFG_OUT_PP_2MHZ    0x2U
#define GPIO_CFG_OUT_PP_10MHZ   0x1U
#define GPIO_CFG_OUT_PP_50MHZ   0x3U
#define GPIO_CFG_OUT_OD_2MHZ    0x6U
#define GPIO_CFG_OUT_OD_50MHZ   0x7U
#define GPIO_CFG_AF_PP_2MHZ     0xAU
#define GPIO_CFG_AF_PP_50MHZ    0xBU   /* USART TX */
#define GPIO_CFG_AF_OD_2MHZ     0xEU
#define GPIO_CFG_AF_OD_50MHZ    0xFU   /* I2C SCL/SDA - must be open drain */

/* ------------------------------------------------------------------------- */
/* AFIO - alternate function remap                                            */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t EVCR;
    volatile uint32_t MAPR;
    volatile uint32_t EXTICR[4];
    volatile uint32_t RESERVED;
    volatile uint32_t MAPR2;
} AFIO_TypeDef;

#define AFIO_BASE       (APB2_BASE + 0x0000UL)  /* 0x40010000 */

#define AFIO_MAPR_I2C1_REMAP        (1UL << 1)
#define AFIO_MAPR_USART2_REMAP      (1UL << 3)
#define AFIO_MAPR_CAN_REMAP_Pos     13U
#define AFIO_MAPR_CAN_REMAP_PA11    (0UL << AFIO_MAPR_CAN_REMAP_Pos)
#define AFIO_MAPR_CAN_REMAP_PB8     (2UL << AFIO_MAPR_CAN_REMAP_Pos)
#define AFIO_MAPR_SWJ_CFG_Pos       24U
#define AFIO_MAPR_SWJ_CFG_Msk       (7UL << AFIO_MAPR_SWJ_CFG_Pos)
/* 010: JTAG disabled, SWD enabled. Frees PB3/PB4/PA15 for GPIO. */
#define AFIO_MAPR_SWJ_CFG_JTAGDISABLE (2UL << AFIO_MAPR_SWJ_CFG_Pos)

/* ------------------------------------------------------------------------- */
/* USART                                                                      */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t SR;        /* 0x00 status  */
    volatile uint32_t DR;        /* 0x04 data    */
    volatile uint32_t BRR;       /* 0x08 baud    */
    volatile uint32_t CR1;       /* 0x0C         */
    volatile uint32_t CR2;       /* 0x10         */
    volatile uint32_t CR3;       /* 0x14         */
    volatile uint32_t GTPR;      /* 0x18         */
} USART_TypeDef;

#define USART1_BASE     (APB2_BASE + 0x3800UL)
#define USART2_BASE     (APB1_BASE + 0x4400UL)  /* 0x40004400 - Nucleo VCP */
#define USART3_BASE     (APB1_BASE + 0x4800UL)

#define USART_SR_PE     (1UL << 0)
#define USART_SR_FE     (1UL << 1)
#define USART_SR_NE     (1UL << 2)
#define USART_SR_ORE    (1UL << 3)
#define USART_SR_IDLE   (1UL << 4)
#define USART_SR_RXNE   (1UL << 5)
#define USART_SR_TC     (1UL << 6)
#define USART_SR_TXE    (1UL << 7)

#define USART_CR1_RE        (1UL << 2)
#define USART_CR1_TE        (1UL << 3)
#define USART_CR1_RXNEIE    (1UL << 5)
#define USART_CR1_TCIE      (1UL << 6)
#define USART_CR1_TXEIE     (1UL << 7)
#define USART_CR1_PCE       (1UL << 10)
#define USART_CR1_M         (1UL << 12)
#define USART_CR1_UE        (1UL << 13)

#define USART_CR3_DMAR      (1UL << 6)
#define USART_CR3_DMAT      (1UL << 7)

/* ------------------------------------------------------------------------- */
/* I2C                                                                        */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t OAR1;      /* 0x08 */
    volatile uint32_t OAR2;      /* 0x0C */
    volatile uint32_t DR;        /* 0x10 */
    volatile uint32_t SR1;       /* 0x14 */
    volatile uint32_t SR2;       /* 0x18 */
    volatile uint32_t CCR;       /* 0x1C */
    volatile uint32_t TRISE;     /* 0x20 */
} I2C_TypeDef;

#define I2C1_BASE       (APB1_BASE + 0x5400UL)  /* 0x40005400 */
#define I2C2_BASE       (APB1_BASE + 0x5800UL)

#define I2C_CR1_PE          (1UL << 0)
#define I2C_CR1_START       (1UL << 8)
#define I2C_CR1_STOP        (1UL << 9)
#define I2C_CR1_ACK         (1UL << 10)
#define I2C_CR1_POS         (1UL << 11)  /* needed for the N=2 receive dance */
#define I2C_CR1_SWRST       (1UL << 15)

#define I2C_CR2_FREQ_Msk    (0x3FUL << 0)

#define I2C_SR1_SB          (1UL << 0)   /* start bit sent      */
#define I2C_SR1_ADDR        (1UL << 1)   /* address ACKed       */
#define I2C_SR1_BTF         (1UL << 2)   /* byte transfer done  */
#define I2C_SR1_RXNE        (1UL << 6)
#define I2C_SR1_TXE         (1UL << 7)
#define I2C_SR1_BERR        (1UL << 8)
#define I2C_SR1_ARLO        (1UL << 9)
#define I2C_SR1_AF          (1UL << 10)  /* acknowledge failure = NACK */
#define I2C_SR1_OVR         (1UL << 11)

#define I2C_SR2_MSL         (1UL << 0)
#define I2C_SR2_BUSY        (1UL << 1)
#define I2C_SR2_TRA         (1UL << 2)

#define I2C_CCR_FS          (1UL << 15)
#define I2C_CCR_DUTY        (1UL << 14)

/* ------------------------------------------------------------------------- */
/* ADC                                                                        */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t SR;        /* 0x00 */
    volatile uint32_t CR1;       /* 0x04 */
    volatile uint32_t CR2;       /* 0x08 */
    volatile uint32_t SMPR1;     /* 0x0C channels 10..17 */
    volatile uint32_t SMPR2;     /* 0x10 channels 0..9   */
    volatile uint32_t JOFR[4];   /* 0x14..0x20 */
    volatile uint32_t HTR;       /* 0x24 */
    volatile uint32_t LTR;       /* 0x28 */
    volatile uint32_t SQR1;      /* 0x2C */
    volatile uint32_t SQR2;      /* 0x30 */
    volatile uint32_t SQR3;      /* 0x34 */
    volatile uint32_t JSQR;      /* 0x38 */
    volatile uint32_t JDR[4];    /* 0x3C..0x48 */
    volatile uint32_t DR;        /* 0x4C */
} ADC_TypeDef;

#define ADC1_BASE       (APB2_BASE + 0x2400UL)  /* 0x40012400 */
#define ADC2_BASE       (APB2_BASE + 0x2800UL)

#define ADC_SR_EOC          (1UL << 1)
#define ADC_CR2_ADON        (1UL << 0)
#define ADC_CR2_CONT        (1UL << 1)
#define ADC_CR2_CAL         (1UL << 2)
#define ADC_CR2_RSTCAL      (1UL << 3)
#define ADC_CR2_EXTTRIG     (1UL << 20)
#define ADC_CR2_EXTSEL_SWSTART (7UL << 17)
#define ADC_CR2_SWSTART     (1UL << 22)
#define ADC_CR2_TSVREFE     (1UL << 23)  /* wakes internal temp sensor + VREFINT */

#define ADC_SMP_239CYC      7U           /* 239.5 cycles - needed for temp sensor */

#define ADC_CH_TEMP         16U
#define ADC_CH_VREFINT      17U

/* ------------------------------------------------------------------------- */
/* General purpose timers (TIM2/3/4)                                          */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t SMCR;      /* 0x08 */
    volatile uint32_t DIER;      /* 0x0C */
    volatile uint32_t SR;        /* 0x10 */
    volatile uint32_t EGR;       /* 0x14 */
    volatile uint32_t CCMR1;     /* 0x18 */
    volatile uint32_t CCMR2;     /* 0x1C */
    volatile uint32_t CCER;      /* 0x20 */
    volatile uint32_t CNT;       /* 0x24 */
    volatile uint32_t PSC;       /* 0x28 */
    volatile uint32_t ARR;       /* 0x2C */
    volatile uint32_t RESERVED1; /* 0x30 */
    volatile uint32_t CCR[4];    /* 0x34..0x40 */
    volatile uint32_t RESERVED2; /* 0x44 */
    volatile uint32_t DCR;       /* 0x48 */
    volatile uint32_t DMAR;      /* 0x4C */
} TIM_TypeDef;

#define TIM2_BASE       (APB1_BASE + 0x0000UL)
#define TIM3_BASE       (APB1_BASE + 0x0400UL)
#define TIM4_BASE       (APB1_BASE + 0x0800UL)

#define TIM_CR1_CEN     (1UL << 0)
#define TIM_EGR_UG      (1UL << 0)
#define TIM_SR_UIF      (1UL << 0)
#define TIM_SR_CC1IF    (1UL << 1)
#define TIM_SR_CC2IF    (1UL << 2)
#define TIM_CCER_CC1E   (1UL << 0)
#define TIM_CCER_CC1P   (1UL << 1)
#define TIM_CCER_CC2E   (1UL << 4)
#define TIM_CCER_CC2P   (1UL << 5)

/* ------------------------------------------------------------------------- */
/* Cortex-M3 core peripherals                                                 */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick_BASE    0xE000E010UL
#define SysTick_CTRL_ENABLE     (1UL << 0)
#define SysTick_CTRL_TICKINT    (1UL << 1)
#define SysTick_CTRL_CLKSOURCE  (1UL << 2)  /* 1 = AHB, 0 = AHB/8 */

typedef struct {
    volatile uint32_t ISER[8];
    uint32_t  RESERVED0[24];
    volatile uint32_t ICER[8];
    uint32_t  RESERVED1[24];
    volatile uint32_t ISPR[8];
    uint32_t  RESERVED2[24];
    volatile uint32_t ICPR[8];
    uint32_t  RESERVED3[24];
    volatile uint32_t IABR[8];
    uint32_t  RESERVED4[56];
    volatile uint8_t  IP[240];
} NVIC_TypeDef;

#define NVIC_BASE       0xE000E100UL

/* DWT - cycle counter, used for sub-microsecond delays */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t CYCCNT;
} DWT_TypeDef;

#define DWT_BASE        0xE0001000UL
#define DWT_CTRL_CYCCNTENA  (1UL << 0)
#define DEMCR           (*(volatile uint32_t *)0xE000EDFCUL)
#define DEMCR_TRCENA    (1UL << 24)

/* ------------------------------------------------------------------------- */
/* Peripheral pointers                                                        */
/* ------------------------------------------------------------------------- */
#ifdef UNIT_TEST
/* Host build: these point at RAM structs defined in test/support/fake_regs.c  */
extern RCC_TypeDef      *const RCC;
extern FLASH_TypeDef    *const FLASHR;
extern GPIO_TypeDef     *const GPIOA;
extern GPIO_TypeDef     *const GPIOB;
extern GPIO_TypeDef     *const GPIOC;
extern AFIO_TypeDef     *const AFIO;
extern USART_TypeDef    *const USART2;
extern I2C_TypeDef      *const I2C1;
extern ADC_TypeDef      *const ADC1;
extern TIM_TypeDef      *const TIM3;
#else
#define RCC     ((RCC_TypeDef   *)RCC_BASE)
#define FLASHR  ((FLASH_TypeDef *)FLASH_R_BASE)
#define GPIOA   ((GPIO_TypeDef  *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef  *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef  *)GPIOC_BASE)
#define AFIO    ((AFIO_TypeDef  *)AFIO_BASE)
#define USART2  ((USART_TypeDef *)USART2_BASE)
#define I2C1    ((I2C_TypeDef   *)I2C1_BASE)
#define ADC1    ((ADC_TypeDef   *)ADC1_BASE)
#define TIM3    ((TIM_TypeDef   *)TIM3_BASE)
#endif

#define SysTickR ((SysTick_TypeDef *)SysTick_BASE)
#define NVICR    ((NVIC_TypeDef    *)NVIC_BASE)
#define DWTR     ((DWT_TypeDef     *)DWT_BASE)

/* ------------------------------------------------------------------------- */
/* Offset self-checks - a wrong struct member breaks the build, not the board */
/* ------------------------------------------------------------------------- */
#define STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond) ? 1 : -1]

STATIC_ASSERT(sizeof(RCC_TypeDef)   == 0x28, rcc_size);
STATIC_ASSERT(sizeof(GPIO_TypeDef)  == 0x1C, gpio_size);
STATIC_ASSERT(sizeof(USART_TypeDef) == 0x1C, usart_size);
STATIC_ASSERT(sizeof(I2C_TypeDef)   == 0x24, i2c_size);
STATIC_ASSERT(sizeof(ADC_TypeDef)   == 0x50, adc_size);
STATIC_ASSERT(sizeof(TIM_TypeDef)   == 0x50, tim_size);

#endif /* STM32F103_H */
