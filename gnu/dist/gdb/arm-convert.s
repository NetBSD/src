	.text
	.global	_convert_from_extended

_convert_from_extended:

	ldfe	f0,[r0]
	stfd	f0,[r1]
	movs	pc,lr

	.global	_convert_to_extended

_convert_to_extended:

	ldfd	f0,[r0]
	stfe	f0,[r1]
	movs	pc,lr
