	.text
	.globl	_start
	.type	_start, @function
_start:
	movl	$call1, %edi
	bnd call *%rdi
	movq	func(%rip), %rdi
	bnd call *%rdi
	ret
	.size	_start, .-_start
	.globl	func
	.data
	.type	func, @object
	.size	func, 8
func:
	.quad	call2
