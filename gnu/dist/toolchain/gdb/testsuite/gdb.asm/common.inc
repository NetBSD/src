	.macro comment text
	.endm

	comment "Can't rely on assembler comment character so do this."

	.macro include arch file
	.include "\arch\file"
	.endm

comment "arch.inc is responsible for defining the following macros:"
comment "enter - subroutine prologue"
comment "leave - subroutine epilogue"
comment "call - call a named subroutine"
comment "several_nops - execute several (typically 4) nops"
comment "exit0 - exit (0)"

comment "macros to label a subroutine may also eventually be needed"
comment "i.e. .global foo\nfoo:\n"
