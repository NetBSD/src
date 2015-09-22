/*	$NetBSD: shl.x,v 1.7.182.1 2015/09/22 12:05:39 skrll Exp $	*/

OUTPUT_FORMAT("elf32-shl-nbsd")
OUTPUT_ARCH(sh)
ENTRY(start)

MEMORY
{
  ram : o = 0x8c001000, l = 16M
}
SECTIONS
{
  .text :
  {
    ftext = . ;
    *(.text)
    *(.rodata)
    *(.strings)
  } > ram
  etext = . ;
  PROVIDE (etext = .);
  . = ALIGN(8);
  .data :
  {
    fdata = . ;
    PROVIDE (fdata = .);
    *(.data)
    CONSTRUCTORS
  } > ram
  edata = . ;
  PROVIDE (edata = .);
  . = ALIGN(8);
  .bss :
  {
    fbss = . ;
    PROVIDE (fbss = .);
    *(.bss)
    *(COMMON)
  } > ram
  . = ALIGN(4);
  end = . ;
  PROVIDE (end = .);
}
