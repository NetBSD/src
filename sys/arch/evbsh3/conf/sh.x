OUTPUT_FORMAT("coff-sh")
OUTPUT_ARCH(sh)
MEMORY
{
  ram : o = 0x8C010000, l = 16M
}
SECTIONS
{
  ROM = 0x80010000;

  .text :
  AT (ROM)
  {
    *(.text)
    *(.strings)
    _etext = . ;
  }  > ram
  .data :
  AT (ROM + SIZEOF(.text))
  {
    *(.data)
    _edata = . ;
  }  > ram
  .bss :
  AT (ROM + SIZEOF(.text) + SIZEOF(.data))
  {
    _bss_start = . ;
    *(.bss)
    *(COMMON)
    _end = . ;
  }  > ram
  .stack   :
  {
    _stack = . ;
    *(.stack)
  }  > ram
  .stab 0 (NOLOAD) :
  {
    *(.stab)
  }
  .stabstr 0 (NOLOAD) :
  {
    *(.stabstr)
  }
}
