	.text
foo:
	.quad 0
	movq	foo@GOTPCREL(%rip), %rax
