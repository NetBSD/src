# Source file used to test -mips4 *non-fp* instructions.

text_label:	
	movn	$4,$6,$6
	movz	$4,$6,$6
	# It used to be disabled due to a clash with lwc3.
	pref	4,0($4)

# Force at least 8 (non-delay-slot) zero bytes, to make 'objdump' print ...
	.align	2
	.space	8
