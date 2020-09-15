# Check ENQCMD[S] 64-bit instructions

	.allow_index_reg
	.text
_start:
	enqcmd (%rcx),%rax
	enqcmd (%ecx),%eax
	enqcmds (%rcx),%rax
	enqcmds (%ecx),%eax

	.intel_syntax noprefix
	enqcmd rax,[rcx]
	enqcmd eax,[ecx]
	enqcmds rax,[rcx]
	enqcmds eax,[ecx]
