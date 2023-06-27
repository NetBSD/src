#define	USERCONF	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_USERCONF
 .global _KERNEL_OPT_USERCONF
 .equiv _KERNEL_OPT_USERCONF,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_USERCONF\n .global _KERNEL_OPT_USERCONF\n .equiv _KERNEL_OPT_USERCONF,0x1\n .endif");
#endif
