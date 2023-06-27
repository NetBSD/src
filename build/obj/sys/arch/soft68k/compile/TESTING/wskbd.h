#define	NWSKBD	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWSKBD
 .global _KERNEL_OPT_NWSKBD
 .equiv _KERNEL_OPT_NWSKBD,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWSKBD\n .global _KERNEL_OPT_NWSKBD\n .equiv _KERNEL_OPT_NWSKBD,0x1\n .endif");
#endif
