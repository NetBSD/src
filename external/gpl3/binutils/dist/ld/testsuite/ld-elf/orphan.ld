SECTIONS
{
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) *(COMMON) }
  .sbss : { *(.sbss) }
  .note : { *(.note) }
  /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.trampolines) }
}
