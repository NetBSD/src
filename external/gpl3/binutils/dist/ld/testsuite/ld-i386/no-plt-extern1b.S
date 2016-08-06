	.text
	.p2align 4,,15
	.globl	get_func
	.type	get_func, @function
get_func:
	movl	func@GOT, %eax
	ret
	.size	get_func, .-get_func
	.p2align 4,,15
	.globl	call_func
	.type	call_func, @function
call_func:
	jmp	*func@GOT
	.size	call_func, .-call_func
	.section	.text.unlikely
	.section	.note.GNU-stack,"",@progbits
