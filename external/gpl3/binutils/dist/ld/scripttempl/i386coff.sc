# Linker script for 386 COFF.  This works on SVR3.2 and SCO Unix 3.2.2.
# Ian Taylor <ian@cygnus.com>.
#
# Copyright (C) 2014-2018 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

test -z "$ENTRY" && ENTRY=_start
# These are substituted in as variables in order to get '}' in a shell
# conditional expansion.
INIT='.init : { *(.init) }'
FINI='.fini : { *(.fini) }'

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
  .text ${RELOCATING+ SIZEOF_HEADERS} : {
    ${RELOCATING+ *(.init)}
    *(.text)
    ${RELOCATING+ *(.fini)}
    ${RELOCATING+ etext  =  .};
  }
  .data ${RELOCATING+ 0x400000 + (. & 0xffc00fff)} : {
    *(.data)
    ${RELOCATING+ edata  =  .};
  }
  .bss ${RELOCATING+ SIZEOF(.data) + ADDR(.data)} :
  {
    *(.bss)
    *(COMMON)
    ${RELOCATING+ end = .};
  }
  ${RELOCATING- ${INIT}}
  ${RELOCATING- ${FINI}}
  .stab  0 ${RELOCATING+(NOLOAD)} :
  {
    [ .stab ]
  }
  .stabstr  0 ${RELOCATING+(NOLOAD)} :
  {
    [ .stabstr ]
  }
}
EOF
