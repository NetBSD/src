# Check MOVDIR[I,64B] 64-bit instructions

	.allow_index_reg
	.text
_start:
	movdiri %rax, (%rcx)
	movdir64b (%rcx),%rax
	movdir64b (%ecx),%eax

	.intel_syntax noprefix
	movdiri [rcx],eax
	movdiri [rcx],rax
	movdiri dword ptr [rcx],eax
	movdiri qword ptr [rcx],rax
	movdir64b rax,[rcx]
	movdir64b eax,[ecx]
