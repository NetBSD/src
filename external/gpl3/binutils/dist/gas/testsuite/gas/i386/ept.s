# Check EPT instructions
	.text
_start:
	invept	(%ecx), %ebx
	invvpid	(%ecx), %ebx

	.intel_syntax noprefix
	invept ebx, oword ptr [ecx]
	invvpid ebx, oword ptr [ecx]
