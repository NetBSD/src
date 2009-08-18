# Source file used to test -mips4 *non-fp* instructions.

text_label:	
	movn	$4,$6,$6
	movz	$4,$6,$6
	# It used to be disabled due to a clash with lwc3.
	pref	4,0($4)

# Round to a 16 byte boundary, for ease in testing multiple targets.
	nop
	nop
	nop
