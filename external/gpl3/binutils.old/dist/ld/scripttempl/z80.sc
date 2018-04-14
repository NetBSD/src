# Copyright (C) 2014-2016 Free Software Foundation, Inc.
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

if [ x${LD_FLAG} = x ]
then
cat << EOF
/* Copyright (C) 2014-2016 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

/* Create a cp/m executable; load and execute at 0x100.  */
OUTPUT_FORMAT("binary")
. = 0x100;
__Ltext = .;
ENTRY (__Ltext)
EOF
else 
    echo "OUTPUT_FORMAT(\"${OUTPUT_FORMAT}\")"
fi
cat <<EOF
OUTPUT_ARCH("${OUTPUT_ARCH}")
SECTIONS
{
.text :	{
	*(.text)
	*(text)
	${RELOCATING+ __Htext = .;}
	}
.data :	{
	${RELOCATING+ __Ldata = .;}
	*(.data)
	*(data)
	${RELOCATING+ __Hdata = .;}
	}
.bss :	{
	${RELOCATING+ __Lbss = .;}
	*(.bss)
	*(bss)
	${RELOCATING+ __Hbss = .;}
	}
}
EOF
