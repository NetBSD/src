#define	NPFSYNC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPFSYNC
 .global _KERNEL_OPT_NPFSYNC
 .equiv _KERNEL_OPT_NPFSYNC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPFSYNC\n .global _KERNEL_OPT_NPFSYNC\n .equiv _KERNEL_OPT_NPFSYNC,0x0\n .endif");
#endif
