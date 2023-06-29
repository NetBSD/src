#define	NSL	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSL
 .global _KERNEL_OPT_NSL
 .equiv _KERNEL_OPT_NSL,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSL\n .global _KERNEL_OPT_NSL\n .equiv _KERNEL_OPT_NSL,0x0\n .endif");
#endif
