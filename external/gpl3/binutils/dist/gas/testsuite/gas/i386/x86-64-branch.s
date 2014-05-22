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
