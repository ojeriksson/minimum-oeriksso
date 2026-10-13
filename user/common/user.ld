OUTPUT_ARCH(arm)
ENTRY(__minemu_user_start)

SECTIONS
{
  . = 0x00400000;
  .text : { *(.text*) }
  . = ALIGN(4096);
  .ARM.exidx : { *(.ARM.exidx*) }
  . = ALIGN(4096);
  .rodata : { *(.rodata*) *(.ARM.extab*) }
  . = ALIGN(4096);
  .data : { *(.data*) }
  . = ALIGN(4096);
  .bss (NOLOAD) : { *(.bss*) *(COMMON) }
}