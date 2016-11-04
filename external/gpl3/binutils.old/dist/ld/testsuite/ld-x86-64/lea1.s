	.section	my_section,"aw",@progbits
	.long	0x12345678
	.text
	.globl	foo
	.type	foo, @function
foo:
	ret
	.size	foo, .-foo
	.globl	_start
	.type	_start, @function
_start:
	movq	foo@GOTPCREL(%rip), %rax
	movq	bar@GOTPCREL(%rip), %rax
	movq	__start_my_section@GOTPCREL(%rip), %rax
	movq	__stop_my_section@GOTPCREL(%rip), %rax
	.size	_start, .-_start
	.comm	pad,4,4
	.comm	bar,4,4
