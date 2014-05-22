defined = addr1;
SECTIONS {
	.text : { *(.text) }
	. = ALIGN (0x1000);
	.data : { *(.data) }
	addr1  = ADDR (.data);
}
