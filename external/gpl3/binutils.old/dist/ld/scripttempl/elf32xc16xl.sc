cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})
${RELOCATING+ENTRY ("_start")}
MEMORY
{
	vectarea : o =0xc00000, l = 0x0300 
	
	introm    : o = 0xc00300, l = 0x16000
	/* The stack starts at the top of main ram.  */
	
	dram   : o = 0x8000 , l = 0xffff
	/* At the very top of the address space is the 8-bit area.  */
         	
         ldata  : o =0x4000 ,l = 0x0200
}
SECTIONS
{
/*.vects :
        {
        *(.vects)
       } ${RELOCATING+ > vectarea} */
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
