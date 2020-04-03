TORS=".tors :
  {
    ___ctors = . ;
    *(.ctors)
    ___ctors_end = . ;
    ___dtors = . ;
    *(.dtors)
    ___dtors_end = . ;
    . = ALIGN(4);
  } > ram"

cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})
${LIB_SEARCH_DIRS}

/* Allow the command line to override the memory region sizes.  */
__PMSIZE = DEFINED(__PMSIZE)  ? __PMSIZE : 256K;
__RAMSIZE = DEFINED(__RAMSIZE) ? __RAMSIZE : 64K;

MEMORY
{
  flash     (rx)   : ORIGIN = 0,        LENGTH = __PMSIZE
  ram       (rw!x) : ORIGIN = 0x800000, LENGTH = __RAMSIZE
}

SECTIONS
{
  .text :
  {
    *(.text*)
    *(.strings)
    *(._pm*)
    *(.init)
    *(.fini)
    ${RELOCATING+ _etext = . ; }
    . = ALIGN(4);
  } ${RELOCATING+ > flash}
  ${CONSTRUCTING+${TORS}}
  .data	: ${RELOCATING+ AT (ADDR (.text) + SIZEOF (.text))}
  {
    *(.data)
    *(.rodata)
    *(.rodata*)
    ${RELOCATING+ _edata = . ; }
    . = ALIGN(4);
  } ${RELOCATING+ > ram}
  .bss  ${RELOCATING+ SIZEOF(.data) + ADDR(.data)} :
  {
    ${RELOCATING+ _bss_start = . ; }
    *(.bss)
    *(COMMON)
    ${RELOCATING+ _end = . ;  }
    . = ALIGN(4);
  } ${RELOCATING+ > ram}

  ${RELOCATING+ __data_load_start = LOADADDR(.data); }
  ${RELOCATING+ __data_load_end = __data_load_start + SIZEOF(.data); }

  .stab 0 ${RELOCATING+(NOLOAD)} :
  {
    *(.stab)
  }
  .stabstr 0 ${RELOCATING+(NOLOAD)} :
  {
    *(.stabstr)
  }
EOF

. $srcdir/scripttempl/DWARF.sc

cat <<EOF
}
EOF
