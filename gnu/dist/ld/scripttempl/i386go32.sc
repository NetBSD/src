# Linker script for i386 go32 (DJGPP)

test -z "$ENTRY" && ENTRY=start
EXE=${CONSTRUCTING+${RELOCATING+-exe}}
cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}${EXE}")

ENTRY(${ENTRY})

SECTIONS
{
  .text ${RELOCATING+ ${TARGET_PAGE_SIZE}+SIZEOF_HEADERS} : {
    *(.text)
    *(.const*)
    *(.ro*)
    ${RELOCATING+etext  =  . ; _etext = .};
    ${RELOCATING+. = ALIGN(${SEGMENT_SIZE});}
  }
  .data ${RELOCATING+ ${DATA_ALIGNMENT}} : {
    ${RELOCATING+djgpp_first_ctor = . ;
    *(.ctor)
    djgpp_last_ctor = . ;}
    ${RELOCATING+djgpp_first_dtor = . ;
    *(.dtor)
    djgpp_last_dtor = . ;}
    *(.data)
    ${RELOCATING+ edata  =  . ; _edata = .};
    ${RELOCATING+ . = ALIGN(${SEGMENT_SIZE});}
  }
  ${CONSTRUCTING+${RELOCATING-.ctor : { *(.ctor) }}}
  ${CONSTRUCTING+${RELOCATING-.dtor : { *(.dtor) }}}
  .bss ${RELOCATING+ SIZEOF(.data) + ADDR(.data)} :
  { 					
    *(.bss)
    *(COMMON)
    ${RELOCATING+ end = . ; _end = .};
    ${RELOCATING+ . = ALIGN(${SEGMENT_SIZE});}
  }
}
EOF
