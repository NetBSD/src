/*	$NetBSD: shl-elf.x,v 1.4 2002/02/24 18:19:44 uch Exp $	*/

OUTPUT_FORMAT("elf32-shl-unx")
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

  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
}

