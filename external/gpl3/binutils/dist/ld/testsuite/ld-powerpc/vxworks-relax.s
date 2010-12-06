	.globl	_start
_start:
	bl	elsewhere
	lis 9,elsewhere@ha
        la 0,elsewhere@l(9)


	.section .far,"ax",@progbits
elsewhere:
	bl	_start

	.section .pad
	.space 0x4000000
