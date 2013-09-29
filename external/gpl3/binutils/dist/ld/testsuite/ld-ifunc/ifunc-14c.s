	.text
	.globl xxx
	.type	xxx, @function
xxx:
	jmp	foo
	.size	xxx, .-xxx
	.hidden	foo
