	.include "common.inc"
	.include "arch.inc"

comment "Second file in assembly source debugging testcase."

	.global foo2
foo2:
	enter

comment "Call someplace else."

	call foo3

comment "All done, return."

	leave
