	.syntax	unified

	.global	_start
	.type	_start, %function
	.text
_start:
	bx	lr
	.size	_start,.-_start

	.section	.data.0,"aw",%progbits
	.align	12
	.type	x, %object
	.size	x, 4
x:
	.word	1

	.section	.data.1,"aw",%progbits
	.align	2

# This causes following relocations to be unaligned.
	.global	padding
	.type	padding, %object
	.size	padding, 1
padding:
	.byte	0

	.global	abs32
	.type	abs32, %object
	.size	abs32, 4
abs32:
	.word	x

	.global	rel32
	.type	rel32, %object
	.size	rel32, 4
rel32:
	.word	x - .

	.global	abs16
	.type	abs16, %object
	.size	abs16, 2
abs16:
	.short	x
	.short	0
