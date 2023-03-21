# Check MOVDIR[I,64B] 32-bit instructions

	.allow_index_reg
	.text
_start:
	.rept 2
	movdiri %eax, (%ecx)
	movdir64b (%ecx),%eax
	movdir64b (%si),%ax

	.intel_syntax noprefix
	movdiri [ecx], eax
	movdiri dword ptr [ecx], eax
	movdir64b eax,[ecx]
	movdir64b ax,[si]

	.att_syntax prefix
	.code16
	.endr

	nop
