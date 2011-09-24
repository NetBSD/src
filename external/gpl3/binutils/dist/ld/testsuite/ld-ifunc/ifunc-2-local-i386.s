	.type foo, %gnu_indirect_function
	.set __GI_foo, foo
	.text
	.type	foo, @function
foo:
	ret
	.size	foo, .-foo
.globl bar
	.type	bar, @function
bar:
	call	.L6
.L6:
	popl	%ebx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-.L6], %ebx
	call	__GI_foo
	leal	__GI_foo@GOTOFF(%ebx), %eax
	ret
	.size	bar, .-bar
