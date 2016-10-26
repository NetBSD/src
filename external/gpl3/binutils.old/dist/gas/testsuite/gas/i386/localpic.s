	.data
foo:
	.long 0
	.text
movl	foo@GOT(%ecx), %eax
