	.file	"main-with-text-rodata.c"
	.global	glob_const_array
	.section	.rodata.glob_const_array,"a",@progbits
	.balign 2
	.type	glob_const_array, @object
	.size	glob_const_array, 20
glob_const_array:
	.short	0
	.short	1
	.short	2
	.short	3
	.short	4
	.short	5
	.short	6
	.short	7
	.short	8
	.short	9
	.section	.text.main,"ax",@progbits
	.balign 2
	.global	main
	.type	main, @function
main:
; start of function
; framesize_regs:     0
; framesize_locals:   2
; framesize_outgoing: 0
; framesize:          2
; elim ap -> fp       2
; elim fp -> sp       2
; saved regs:(none)
	; start of prologue
	SUB.W	#2, R1
	; end of prologue
	MOV.W	#0, @R1
	BR	#.L2
.L5:
	MOV.W	@R1, R12
	ADD.W	R12, R12
	ADD.W	#glob_const_array, R12
	MOV.W	@R12, R13
	MOV.W	R13, R12
	ADD.W	R12, R12
	ADD.W	R13, R12
	rpt	#2 { rlax.w	R12
	SUB.W	R13, R12
	CMP.W	#110, R12 { JNE	.L3
.L4:
	BR	#.L4
.L3:
	ADD.W	#1, @R1
.L2:
	MOV.B	#9, R12
	CMP.W	@R1, R12 { JGE	.L5
	MOV.B	#0, R12
	; start of epilogue
	.refsym	__crt0_call_exit
	ADD.W	#2, R1
	RET
	.size	main, .-main
