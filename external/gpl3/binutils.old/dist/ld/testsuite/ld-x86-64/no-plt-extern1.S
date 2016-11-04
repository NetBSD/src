	.text
	.p2align 4,,15
	.globl	get_func
	.type	get_func, @function
get_func:
	movq	func@GOTPCREL(%rip), %rax
	ret
	.size	get_func, .-get_func
	.p2align 4,,15
	.globl	call_func
	.type	call_func, @function
call_func:
	jmp	*func@GOTPCREL(%rip)
	.size	call_func, .-call_func
	.section	.note.GNU-stack,"",@progbits
