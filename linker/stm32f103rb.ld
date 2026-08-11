/*
 * Linker script for STM32F103RB: 128 KB flash at 0x08000000, 20 KB SRAM at
 * 0x20000000.
 *
 * The one idea worth understanding here is LMA vs VMA. Initialised globals have
 * to *live* in flash (they must survive power off) but be *used* from RAM (they
 * must be writable). AT> FLASH puts the load image in flash while > RAM sets the
 * addresses the code uses; _sidata marks where the flash copy begins so the
 * startup code knows what to copy where.
 */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

/* Stack grows down from the top of RAM. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x000;    /* no heap - nothing here calls malloc */
_Min_Stack_Size = 0x400;    /* 1 KB, checked by the assert below   */

SECTIONS
{
  /* The vector table must be the very first thing in flash. */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .ARM.extab : { *(.ARM.extab*) } >FLASH
  .ARM.exidx : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } >FLASH

  /* _sidata: where the .data load image sits in flash. */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* Build fails rather than mysteriously overflowing the stack at runtime. */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ : { libc.a(*) libm.a(*) libgcc.a(*) }
  .ARM.attributes 0 : { *(.ARM.attributes) }
}
