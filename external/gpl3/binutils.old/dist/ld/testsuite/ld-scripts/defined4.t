SECTIONS {
	.text : { *(.text) }
	.data : { *(.data) }
	.bss : { *(.bss) *(COMMON) }
}
defined1 = defined;
