	.include "common.inc"
	.include "arch.inc"

comment "main routine for assembly source debugging test"
comment "This particular testcase uses macros in <arch>.inc to achieve"
comment "machine independence.  This file must be compiled with -Darch=foo."

comment "WARNING: asm-source.exp checks for line numbers printed by gdb,"
comment "therefore be careful about changing this file without also changing"
comment "asm-source.exp."

	.global main
main:
	enter

comment "Call a macro that consists of several lines of assembler code."

	several_nops

comment "Call a subroutine in another file."

	call foo2

comment "All done."

	exit0

comment "A routine for foo2 to call."

	.global foo3
foo3:
	enter
	leave

	.global exit
exit:
	exit0
