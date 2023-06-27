#define	NLM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NLM
 .global _KERNEL_OPT_NLM
 .equiv _KERNEL_OPT_NLM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NLM\n .global _KERNEL_OPT_NLM\n .equiv _KERNEL_OPT_NLM,0x0\n .endif");
#endif
