#define	NATAPIBUS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NATAPIBUS
 .global _KERNEL_OPT_NATAPIBUS
 .equiv _KERNEL_OPT_NATAPIBUS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NATAPIBUS\n .global _KERNEL_OPT_NATAPIBUS\n .equiv _KERNEL_OPT_NATAPIBUS,0x0\n .endif");
#endif
