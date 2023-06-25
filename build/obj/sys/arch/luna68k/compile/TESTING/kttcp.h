#define	NKTTCP	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NKTTCP
 .global _KERNEL_OPT_NKTTCP
 .equiv _KERNEL_OPT_NKTTCP,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NKTTCP\n .global _KERNEL_OPT_NKTTCP\n .equiv _KERNEL_OPT_NKTTCP,0x0\n .endif");
#endif
