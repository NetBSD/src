.text
	callq	*%rax
	call	*%rax
	call	*%ax
	callw	*%ax
	callw	*(%rax)
	jmpq	*%rax
	jmp	*%rax
	jmp	*%ax
	jmpw	*%ax
	jmpw	*(%rax)
	call	0x100040
	jmp	0x100040

	.byte 0x66
	call	foo
	.byte 0x66
	jmp	foo
	.byte 0x66
	jb	foo

	.intel_syntax noprefix
	call	rax
	callq	rax
	call	ax
	callw	ax
	callw	[rax]
	jmp	rax
	jmpq	rax
	jmp	ax
	jmpw	ax
	jmpw	[rax]
	call	0x100040
	jmp	0x100040
