# Copyright (C) 2014-2016 Free Software Foundation, Inc.
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

cat <<EOF
/* Copyright (C) 2014-2016 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

SECTIONS
{ 
    .text : 
    { 
	${GLD_STYLE+ CREATE_OBJECT_SYMBOLS}
	*(.text) 
	${RELOCATING+ _etext = .};
	${CONSTRUCTING+${COFF_CTORS}}
    }  
    .data :
    { 
 	*(.data) 
	${CONSTRUCTING+CONSTRUCTORS}
	${RELOCATING+ _edata = .};
    }  
    .bss :
    { 
	${RELOCATING+ _bss_start = .};
	*(.bss)	 
	*(COMMON) 
	${RELOCATING+ _end = .};
    } 
} 
EOF
