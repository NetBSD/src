TORS=".tors :
  {
    ___ctors = . ;
    *(.ctors)
    ___ctors_end = . ;
    ___dtors = . ;
    *(.dtors)
    ___dtors_end = . ;
  }"


cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})
${LIB_SEARCH_DIRS}

SECTIONS
{
  . = ${TEXT_START_ADDR} + SIZEOF_HEADERS;
  .text ALIGN(0x10):
  {
    *(.text)
    *(.strings)
    ${RELOCATING+ _etext = . ; }
  }
  ${CONSTRUCTING+${TORS}}
  .data  ${RELOCATING+ ALIGN(${TARGET_PAGE_SIZE})} :
  {
    *(.data)
    ${RELOCATING+ _edata = . ; }
  }
  .bss ${RELOCATING+ ALIGN(${TARGET_PAGE_SIZE})} :
  {
    ${RELOCATING+ _bss_start = . ; }
    *(.bss)
    *(COMMON)
    ${RELOCATING+ _end = . ;  }
  }
  .stack :
  {
    ${RELOCATING+ _stack = . ; }
    *(.stack)
  }
  .stab 0 ${RELOCATING+(NOLOAD)} :
  {
    *(.stab)
  }
  .stabstr 0 ${RELOCATING+(NOLOAD)} :
  {
    *(.stabstr)
  }
}
EOF
