# Test the 64bit instructions of mips16e.

	.text
	.set	mips16
foo:
	sew	$4
	zew	$4

	.p2align 4
