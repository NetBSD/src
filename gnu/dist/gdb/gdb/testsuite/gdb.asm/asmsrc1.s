	.include "common.inc"
	.include "arch.inc"

comment "WARNING: asm-source.exp checks for line numbers printed by gdb."
comment "Be careful about changing this file without also changing"
comment "asm-source.exp."

	
comment	"This file is not linked with crt0."
comment	"Provide very simplistic equivalent."
	
	.global _start
_start:
	gdbasm_startup
	gdbasm_call main
	gdbasm_exit0


comment "main routine for assembly source debugging test"
comment "This particular testcase uses macros in <arch>.inc to achieve"
comment "machine independence."

	.global main
main:
	gdbasm_enter

comment "Call a macro that consists of several lines of assembler code."

	gdbasm_several_nops

comment "Call a subroutine in another file."

	gdbasm_call foo2

comment "All done."

	gdbasm_exit0

comment "A routine for foo2 to call."

	.global foo3
foo3:
	gdbasm_enter
	gdbasm_leave

	.global exit
exit:
	gdbasm_exit0

comment "A static function"

foostatic:
	gdbasm_enter
	gdbasm_leave

comment "A global variable"

	.global globalvar
gdbasm_datavar	globalvar	11

comment "A static variable"

gdbasm_datavar	staticvar	5
