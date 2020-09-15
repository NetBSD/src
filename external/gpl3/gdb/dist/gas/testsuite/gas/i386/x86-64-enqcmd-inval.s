# Check error for ENQCMD[S] 32-bit instructions

	.allow_index_reg
	.text
_start:
	enqcmd (%esi),%rax
	enqcmd (%rsi),%eax
	enqcmds (%esi),%rax
	enqcmds (%rsi),%eax

	.intel_syntax noprefix
	enqcmd rax,[esi]
	enqcmd eax,[rsi]
	enqcmds rax,[esi]
	enqcmds eax,[rsi]
