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
