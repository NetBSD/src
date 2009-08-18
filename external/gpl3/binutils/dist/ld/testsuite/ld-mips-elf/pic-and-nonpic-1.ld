SECTIONS
{
  . = 0x40000;
  .reginfo : { *(.reginfo) }
  .pdr : { *(.pdr) }
  . = 0x41000;
  .text : { *(.text) }
  _gp = 0x68000;
}
