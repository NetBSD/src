	.text
        .type foo, @function
	.global
foo:
        movl xxx(%rip), %eax
        ret

	.data
xxx:
	.quad ifunc 
