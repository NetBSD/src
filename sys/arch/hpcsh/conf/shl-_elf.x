/*	$NetBSD: shl-_elf.x,v 1.1.2.1 2002/01/10 19:44:19 thorpej Exp $	*/

OUTPUT_FORMAT("elf32-shl-unx")
OUTPUT_ARCH(sh)
ENTRY(_start)

MEMORY
{
  ram : o = 0x8c001000, l = 16M
}
SECTIONS
{
  _trap_base	= 0x8c000000;
  PROVIDE (_trap_base = .);
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

  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
}

