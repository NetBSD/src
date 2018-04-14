# Copyright (C) 2014-2018 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

cat <<EOF
/* Copyright (C) 2014-2018 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}")
${LIB_SEARCH_DIRS}
PROVIDE (__stack = 0);
SECTIONS
{
  .text ${RELOCATING+ 0x1000000} : {
    *(.text)
    ${CONSTRUCTING+ . = ALIGN(4);}
    ${RELOCATING+ etext  =  .;}
    ${CONSTRUCTING+ __CTOR_LIST__ = .;}
    ${CONSTRUCTING+ LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)}
    ${CONSTRUCTING+ *(.ctors)}
    ${CONSTRUCTING+ LONG(0)}
    ${CONSTRUCTING+ __CTOR_END__ = .;}
    ${CONSTRUCTING+ __DTOR_LIST__ = .;}
    ${CONSTRUCTING+ LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)}
    ${CONSTRUCTING+ *(.dtors)}
    ${CONSTRUCTING+ LONG(0)}
    ${CONSTRUCTING+ __DTOR_END__ = .;}
  }
  .data : {
    *(.data)
    ${RELOCATING+ edata  =  .};
  }
  .bss : {
    ${RELOCATING+ __bss_start = .};
    *(.bss)
    *(COMMON)
     ${RELOCATING+ end = ALIGN(0x8)};
     ${RELOCATING+ _end = ALIGN(0x8)};
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
