# Source file used to test the jal macro with -KPIC code.

	.weak	weak_text_label

	.ent	text_label
text_label:
	.frame	$sp,0,$31
	.set	noreorder
	.cpload	$25
	.cprestore	0
	jal	$25
	jal	$4,$25
	jal	text_label
	jal	weak_text_label
	jal	external_text_label

# Test j as well.
	j	text_label
	.set	reorder
	nop
	.end	text_label

# Force at least 8 (non-delay-slot) zero bytes, to make 'objdump' print ...
	.align	2
	.space	8
