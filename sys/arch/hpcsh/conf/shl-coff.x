/*	$NetBSD: shl-coff.x,v 1.4.12.2 2002/02/24 18:19:45 uch Exp $	*/

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
