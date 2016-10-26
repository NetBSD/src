# Check FSGSBase new instructions.

	.text
foo:
	rdfsbase %ebx
	rdgsbase %ebx
	wrfsbase %ebx
	wrgsbase %ebx

	.intel_syntax noprefix
	rdfsbase ebx
	rdgsbase ebx
	wrfsbase ebx
	wrgsbase ebx
