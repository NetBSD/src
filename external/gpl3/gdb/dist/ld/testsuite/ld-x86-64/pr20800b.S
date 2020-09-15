	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"PASS\n"
	.text
	.globl	bar
	.type	bar, @function
bar:
.L2:
	leaq	.L2(%rip), %rdx
	movabsq	$_GLOBAL_OFFSET_TABLE_-.L2, %r11
	movabsq	$.LC0@GOTOFF, %rax
	pushq	%r15
	movabsq	$printf@PLTOFF, %rcx
	addq	%r11, %rdx
	popq	%r15
	leaq	(%rdx,%rax), %rdi
	addq	%rdx, %rcx
	xorl	%eax, %eax
	jmp	*%rcx
	.size	bar, .-bar
	.section	.note.GNU-stack,"",@progbits
