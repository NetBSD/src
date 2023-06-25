#define	NPF	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPF
 .global _KERNEL_OPT_NPF
 .equiv _KERNEL_OPT_NPF,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPF\n .global _KERNEL_OPT_NPF\n .equiv _KERNEL_OPT_NPF,0x0\n .endif");
#endif
