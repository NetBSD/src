# Copyright (C) 2014-2018 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

TORS="
    ___ctors = . ;
    *(.ctors)
    ___ctors_end = . ;
    ___dtors = . ;
    *(.dtors)
    ___dtors_end = . ;"

cat <<EOF
/* Copyright (C) 2014-2018 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})

/* Code and data 64k total */

SECTIONS
{
.text ${RELOCATING+ 0x0000} :
	{
	  *(.text)
	  ${RELOCATING+ _etext = . ; }
	}

.data  ${RELOCATING+ . } :
	{
	  *(.data)
	  ${RELOCATING+ _edata = . ; }
	}

.rdata  ${RELOCATING+ . } :
	{
	  *(.rdata);
	  *(.strings)

	  ${CONSTRUCTING+${TORS}}
	}

.bss  ${RELOCATING+ . } :
	{
	  ${RELOCATING+ __start_bss = . ; }
	  *(.bss)
	  *(COMMON)
	  ${RELOCATING+ _end = . ;  }
	}

.stack  ${RELOCATING+ 0xfff0} :
	{
	  ${RELOCATING+ _stack = . ; }
	  *(.stack)
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
