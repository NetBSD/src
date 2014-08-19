# Source file used to check the lack of branch swapping with a relaxed macro.

	.text
foo:
	la	$2, bar
	jr	$3

	la	$2, bar
	beqz	$3, 0f
0:

# Force at least 8 (non-delay-slot) zero bytes, to make 'objdump' print ...
	.align	2
	.space	8
