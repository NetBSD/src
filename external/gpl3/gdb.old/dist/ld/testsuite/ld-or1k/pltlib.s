	.section .data
	.globl x, y
x:	.long 33
y:	.long 44

	.section .text
	.align 4
	.global	func
	.type	func, @function
func:
	l.jr	r9
	 l.nop
