	.text
	.globl foo
	.type foo, @function
foo:
	.cfi_startproc
	call	func1@plt
	call	func2@plt
	movq	func1@GOTPCREL(%rip), %rax
	.cfi_endproc
