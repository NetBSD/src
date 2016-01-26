# Check xsave/xrstor
	.text
_start:
	xrstor		(%ebx)
	xsave		(%ebx)
	xsaveopt	(%ebx)
	xgetbv
	xsetbv

	.intel_syntax noprefix
	xrstor		[ecx]
	xsave		[ecx]
	xsaveopt	[ecx]
