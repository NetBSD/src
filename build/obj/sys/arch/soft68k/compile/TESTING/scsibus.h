#define	NSCSIBUS	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSCSIBUS
 .global _KERNEL_OPT_NSCSIBUS
 .equiv _KERNEL_OPT_NSCSIBUS,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSCSIBUS\n .global _KERNEL_OPT_NSCSIBUS\n .equiv _KERNEL_OPT_NSCSIBUS,0x1\n .endif");
#endif
