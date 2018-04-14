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
OUTPUT_ARCH(${ARCH})
${RELOCATING+ENTRY (_start)}
${RELOCATING+${LIB_SEARCH_DIRS}}

SECTIONS
{
  .text ${RELOCATING+ 0x2000 + SIZEOF_HEADERS} :
    {
      ${RELOCATING+ __.text.start = .};
      *(.text)
      ${RELOCATING+ etext  =  .;}
      ${RELOCATING+ _etext  =  .;}
      ${RELOCATING+ __.text.end = .};
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
  .data ${RELOCATING+ SIZEOF(.text) + ADDR(.text) + 0x400000} :
    {
      ${RELOCATING+ __.data.start = .};
      *(.data)
      ${RELOCATING+ edata  =  .};
      ${RELOCATING+ _edata  =  .};
      ${RELOCATING+ __.data.end = .};
    }
  .bss ${RELOCATING+ SIZEOF(.data) + ADDR(.data)} :
    {
      ${RELOCATING+ __.bss.start = .};
      *(.bss)
      *(COMMON)
      ${RELOCATING+ __.bss.end = .};
      ${RELOCATING+ end = ALIGN(0x8)};
      ${RELOCATING+ _end = ALIGN(0x8)};
    }
  .comment ${RELOCATING+ 0} :
    {
      *(.comment)
    }
}
EOF
