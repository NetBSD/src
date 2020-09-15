	.text
	.globl foo
	.type foo, @function
foo:
	.cfi_startproc
	call	func@plt
	.cfi_endproc
