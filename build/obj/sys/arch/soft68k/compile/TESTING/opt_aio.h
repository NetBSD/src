#define	AIO	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_AIO
 .global _KERNEL_OPT_AIO
 .equiv _KERNEL_OPT_AIO,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_AIO\n .global _KERNEL_OPT_AIO\n .equiv _KERNEL_OPT_AIO,0x1\n .endif");
#endif
