#ifdef __mips__
	.abicalls
	.bss
	.align 2
	.space 2
	.globl	mbr
mbr:	.space 512
#endif
