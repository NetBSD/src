# Source file used to test the add macro.
	
foo:
	.ifndef r6
	add	$4,$4,0
	add	$4,$4,1
	add	$4,$4,0x8000
	add	$4,$4,-0x8000
	add	$4,$4,0x10000
	add	$4,$4,0x1a5a5
	.endif

# addu is handled the same way add is; just confirm that it isn't
# totally broken.
	addu	$4,$4,1

	.align 2
	.space 8
