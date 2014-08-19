	.text
	.globl	_start
	.globl	__start
_start:
__start:
	.comm	foo, 4, 4
	.section .debug_foo,"",%progbits
	.balign	16
	.ifdef	ELF64
	.8byte	foo
	.else
	.4byte	foo
	.endif
	.balign	16
