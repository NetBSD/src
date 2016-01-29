	.text
	.globl	main
	.type	main, @function
main:
	subl	$12, %esp
	call	*foo@GOT
	call	*bar@GOT
	call	*plt@GOT
	movl	foo@GOT, %eax
	call	*%eax
	movl	bar@GOT, %eax
	call	*%eax
	movl	plt@GOT, %eax
	call	*%eax
	movl	foo@GOT(%ebx), %ecx
	call	*%ecx
	subl	$12, %esp
	pushl	$0
	pushl	$0	# Push a dummy return address onto stack.
	jmp	*myexit@GOT
	.size	main, .-main
