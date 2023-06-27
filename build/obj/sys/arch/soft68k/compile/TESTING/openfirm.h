#define	NOPENFIRM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NOPENFIRM
 .global _KERNEL_OPT_NOPENFIRM
 .equiv _KERNEL_OPT_NOPENFIRM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NOPENFIRM\n .global _KERNEL_OPT_NOPENFIRM\n .equiv _KERNEL_OPT_NOPENFIRM,0x0\n .endif");
#endif
