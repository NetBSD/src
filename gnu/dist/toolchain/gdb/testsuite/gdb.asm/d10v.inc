	comment "subroutine prologue"
	.macro enter
	st r13,@-sp
	.endm

	comment "subroutine epilogue"
	.macro leave
	ld r13,@sp+
	jmp r13
	.endm

	.macro call subr
	bl \subr
	.endm

	.macro several_nops
	nop
	nop
	nop
	nop
	.endm

	comment "exit (0)"
	.macro exit0
	ldi r4, 1
	ldi r0, 0
	trap 15
	.endm
