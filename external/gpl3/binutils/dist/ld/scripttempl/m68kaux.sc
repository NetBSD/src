# Linker script for A/UX.
#
# Copyright (C) 2014-2018 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

test -z "$ENTRY" && ENTRY=_start
INIT='.init : { *(.init) }'
FINI='.fini : { *(.fini) }'
CTORS='.ctors : { *(.ctors) }'
DTORS='.dtors : { *(.dtors) }'

cat <<EOF
/* Copyright (C) 2014-2018 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}")
${LIB_SEARCH_DIRS}

${RELOCATING+ENTRY (${ENTRY})}

SECTIONS
{
  .text ${RELOCATING+ $TEXT_START_ADDR} : {
    ${RELOCATING+ *(.init)}
    ${RELOCATING+ *(.fini)}
    *(.text)
    ${RELOCATING+ . = ALIGN(4);}
    ${RELOCATING+ *(.ctors)}
    ${RELOCATING+ *(.dtors)}
    ${RELOCATING+ etext = .;}
    ${RELOCATING+ _etext = .;}
  } =0x4E714E71
  .data ${RELOCATING+ $DATA_ALIGNMENT} : {
    *(.data)
    ${RELOCATING+ edata = .;}
    ${RELOCATING+ _edata = .;}
  }
  .bss : {
    *(.bss)
    *(COMMON)
    ${RELOCATING+ end = .;}
    ${RELOCATING+ _end = .;}
  }
  ${RELOCATING- ${INIT}}
  ${RELOCATING- ${FINI}}
  ${RELOCATING- ${CTORS}}
  ${RELOCATING- ${DTORS}}

  .comment 0 ${RELOCATING+(NOLOAD)} : { [ .comment ] [ .ident ] }
  .stab 0 ${RELOCATING+(NOLOAD)} : { [ .stab ] }
  .stabstr 0 ${RELOCATING+(NOLOAD)} : { [ .stabstr ] }
}
EOF
