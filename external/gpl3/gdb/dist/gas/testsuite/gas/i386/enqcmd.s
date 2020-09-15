# Check ENQCMD[S] 32-bit instructions

	.allow_index_reg
	.text
_start:
	enqcmd (%ecx),%eax
	enqcmd (%si),%ax
	enqcmds (%ecx),%eax
	enqcmds (%si),%ax

	.intel_syntax noprefix
	enqcmd eax,[ecx]
	enqcmd ax,[si]
	enqcmds eax,[ecx]
	enqcmds ax,[si]
