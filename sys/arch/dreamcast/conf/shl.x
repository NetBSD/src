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
  }  > ram
  .bss :
  {
     bss_start = . ; 
    *(.bss)
    *(COMMON)
     end = . ;  
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
