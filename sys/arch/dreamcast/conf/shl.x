OUTPUT_FORMAT("elf32-shl-unx")
OUTPUT_ARCH(sh)
MEMORY
{
  ram : o = 0x8C010000, l = 16M
}
SECTIONS
{
  .text :
  {
    *(.text)
    *(.rodata)
    *(.strings)
     etext = . ; 
     _etext = . ;  /* XXX */
  }  > ram
  .tors :
  {
    __ctors = . ;
    *(.ctors)
    __ctors_end = . ;
    __dtors = . ;
    *(.dtors)
    __dtors_end = . ;
  } > ram
  .data :
  {
    *(.data)
     edata = . ; 
     _edata = . ;  /* XXX */
  }  > ram
  .bss :
  {
     bss_start = . ; 
    *(.bss)
    *(COMMON)
     end = . ;  
     _end = . ;  /* XXX */
  }  > ram
  .stack   :
  {
     stack = . ; 
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
