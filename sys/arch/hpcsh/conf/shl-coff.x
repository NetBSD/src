/*	$NetBSD: shl-coff.x,v 1.2.2.2 2002/03/16 15:58:04 jdolecek Exp $	*/

OUTPUT_FORMAT("coff-shl")
OUTPUT_ARCH(sh)

MEMORY
{
  ram : o = 0x8C001000, l = 16M
}

SECTIONS
{
  .text :
  {
    _ftext = . ;
    *(.text)
    *(.rodata)
    *(.strings)
  } > ram
  _etext = . ;
  PROVIDE (_etext = .);
  . = ALIGN(8);
  .data :
  {
    _fdata = . ;
    PROVIDE (_fdata = .);
    *(.data)
    CONSTRUCTORS
  } > ram
  _edata = . ;
  PROVIDE (_edata = .);
  . = ALIGN(8);
  .bss :
  {
    _fbss = . ;
    PROVIDE (_fbss = .);
    *(.bss)
    *(COMMON)
  } > ram
  . = ALIGN(4);
  _end = . ;
  PROVIDE (_end = .);
}
