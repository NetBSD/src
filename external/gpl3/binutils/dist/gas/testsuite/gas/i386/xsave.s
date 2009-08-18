# Check xsave/xrstor
	.text
_start:
	xrstor		(%ebx)
	xsave		(%ebx)
	xgetbv
	xsetbv

	.intel_syntax noprefix
	xrstor		[ecx]
	xsave		[ecx]
