	.text
	.globl bar
	.type	bar, @function
bar:
	jmp	foo
	.size	bar, .-bar
	.hidden	foo
