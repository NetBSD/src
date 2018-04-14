# Copyright (C) 2014-2016 Free Software Foundation, Inc.
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
/* Copyright (C) 2014-2016 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})

/* Code and data, both larger than 64k */

SECTIONS
{
.text ${RELOCATING+ 0x10000} :
	{
	  *(.text)
	  ${RELOCATING+ _etext = . ; }
	}

.data  ${RELOCATING+ 0x20000} :
	{
	  *(.data)
	  ${RELOCATING+ _edata = . ; }
	}

.rdata  ${RELOCATING+ 0x30000} :
	{
	  *(.rdata);
	  *(.strings)

	  ${CONSTRUCTING+${TORS}}
	}

.bss  ${RELOCATING+ 0x40000} :
	{
	  ${RELOCATING+ __start_bss = . ; }
	  *(.bss)
	  *(COMMON)
	  ${RELOCATING+ _end = . ;  }
	}

.stack  ${RELOCATING+ 0x50000} :
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
