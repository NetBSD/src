# Copyright (C) 2014-2020 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

cat <<EOF
/* Copyright (C) 2014-2020 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}", "${BIG_OUTPUT_FORMAT}",
	      "${LITTLE_OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})

${RELOCATING+${LIB_SEARCH_DIRS}}
SECTIONS
{
  ${RELOCATING+. = ${TEXT_START_ADDR};}
  .text :
  {
    CREATE_OBJECT_SYMBOLS
    *(.text)
    ${RELOCATING+etext = ${DATA_ALIGNMENT};}
  }
  ${RELOCATING+. = ${DATA_ALIGNMENT};}
  .data :
  {
    *(.data)
    ${CONSTRUCTING+CONSTRUCTORS}
    ${RELOCATING+edata  =  .;}
  }
  .bss :
  {
   *(.bss)
   *(COMMON)
   ${RELOCATING+end = . };
  }
}
EOF
