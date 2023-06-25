#define	NATPPC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NATPPC
 .global _KERNEL_OPT_NATPPC
 .equiv _KERNEL_OPT_NATPPC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NATPPC\n .global _KERNEL_OPT_NATPPC\n .equiv _KERNEL_OPT_NATPPC,0x0\n .endif");
#endif
