SECTIONS {
	 .text : { *(.text) } >rom
	 INCLUDE include-data.t
	 /DISCARD/ : { *(*) }
}
