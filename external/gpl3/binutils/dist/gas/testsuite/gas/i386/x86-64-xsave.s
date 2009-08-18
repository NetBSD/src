# Check 64bit xsave/xrstor
	.text
_start:
	xrstor		(%r9)
	xsave		(%r9)
	xgetbv
	xsetbv

	.intel_syntax noprefix
	xrstor		[rcx]
	xsave		[rcx]
