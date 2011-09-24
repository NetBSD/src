SECTIONS
{
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) *(COMMON) }
  .note : { *(.note) }
  /DISCARD/ : { *(.reginfo) *(.trampolines) }
}
