#define	NPFLOG	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPFLOG
 .global _KERNEL_OPT_NPFLOG
 .equiv _KERNEL_OPT_NPFLOG,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPFLOG\n .global _KERNEL_OPT_NPFLOG\n .equiv _KERNEL_OPT_NPFLOG,0x0\n .endif");
#endif
