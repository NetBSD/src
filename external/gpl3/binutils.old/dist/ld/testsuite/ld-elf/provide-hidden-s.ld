SECTIONS
{
  . = 0x12300000;
  .data : { *(.data) }
  .got : { *(.got) }
  .interp : { *(.interp) }
  .dynsym : { *(.dynsym) }
  .dynstr : { *(.dynstr) }
  .dynamic : { *(.dynamic) }
  .hash : { *(.hash) }
}
