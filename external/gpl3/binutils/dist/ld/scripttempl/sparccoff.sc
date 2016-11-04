# Linker script for Sparc COFF.
# Based on i386coff.sc by Ian Taylor <ian@cygnus.com>.
#
# Copyright (C) 2014-2016 Free Software Foundation, Inc.
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

test -z "$ENTRY" && ENTRY=_start

cat <<EOF
/* Copyright (C) 2014-2016 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}")
${LIB_SEARCH_DIRS}

${RELOCATING+ENTRY (${ENTRY})}

SECTIONS
{
  .text ${RELOCATING+ SIZEOF_HEADERS} : {
    *(.init)
    *(.text)
    ${RELOCATING+ etext  =  .;}
    ${CONSTRUCTING+ ___CTOR_LIST__ = .;}
    ${CONSTRUCTING+ LONG((___CTOR_END__ - ___CTOR_LIST__) / 4 - 2)}
    ${CONSTRUCTING+ *(.ctors)}
    ${CONSTRUCTING+ LONG(0)}
    ${CONSTRUCTING+ ___CTOR_END__ = .;}
    ${CONSTRUCTING+ ___DTOR_LIST__ = .;}
    ${CONSTRUCTING+ LONG((___DTOR_END__ - ___DTOR_LIST__) / 4 - 2)}
    ${CONSTRUCTING+ *(.dtors)}
    ${CONSTRUCTING+ LONG(0)}
    ${CONSTRUCTING+ ___DTOR_END__ = .;}
    *(.fini)
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
