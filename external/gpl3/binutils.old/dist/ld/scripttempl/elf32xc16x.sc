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

OUTPUT_FORMAT("${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})
${RELOCATING+ENTRY ("_start")}
MEMORY
{
	
	vectarea : o =0x00000, l = 0x0300 
	
	introm    : o = 0x00400, l = 0x16000
	/* The stack starts at the top of main ram.  */
	
	dram   : o = 0x8000 , l = 0xffff
	/* At the very top of the address space is the 8-bit area.  */
         	
         ldata  : o =0x4000 ,l = 0x0200
}

SECTIONS
{
.init :
        {
          *(.init)
        } ${RELOCATING+ >introm}
 
.text :
	{
	  *(.rodata) 
	  *(.text.*)
	  *(.text)
	  	  ${RELOCATING+ _etext = . ; }
	} ${RELOCATING+ > introm}
.data :
	{
	  *(.data)
	  *(.data.*)
	  
	  ${RELOCATING+ _edata = . ; }
	} ${RELOCATING+ > dram}

.bss :
	{
	  ${RELOCATING+ _bss_start = . ;}
	  *(.bss)
	  *(COMMON)
	  ${RELOCATING+ _end = . ;  }
	} ${RELOCATING+ > dram}

 .ldata :
         {
          *(.ldata)
         } ${RELOCATING+ > ldata}

  
  .vects :
          {
          *(.vects)
       } ${RELOCATING+ > vectarea}     

}
EOF
