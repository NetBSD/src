#define	SYSVSHM	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SYSVSHM
 .global _KERNEL_OPT_SYSVSHM
 .equiv _KERNEL_OPT_SYSVSHM,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SYSVSHM\n .global _KERNEL_OPT_SYSVSHM\n .equiv _KERNEL_OPT_SYSVSHM,0x1\n .endif");
#endif
#define	SYSVSEM	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SYSVSEM
 .global _KERNEL_OPT_SYSVSEM
 .equiv _KERNEL_OPT_SYSVSEM,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SYSVSEM\n .global _KERNEL_OPT_SYSVSEM\n .equiv _KERNEL_OPT_SYSVSEM,0x1\n .endif");
#endif
#define	SYSVMSG	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SYSVMSG
 .global _KERNEL_OPT_SYSVMSG
 .equiv _KERNEL_OPT_SYSVMSG,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SYSVMSG\n .global _KERNEL_OPT_SYSVMSG\n .equiv _KERNEL_OPT_SYSVMSG,0x1\n .endif");
#endif
