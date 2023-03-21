	.text
	.globl	main
	.type	main, @function
main:
.L2:
	leaq	.L2(%rip), %rax
	movabsq	$_GLOBAL_OFFSET_TABLE_-.L2, %r11
	movabsq	$bar@PLTOFF, %rdx
	pushq	%r15
	addq	%r11, %rax
	movq	%rax, %r15
	addq	%rax, %rdx
	call	*%rdx
	xorl	%eax, %eax
	popq	%r15
	ret
	.size	main, .-main
