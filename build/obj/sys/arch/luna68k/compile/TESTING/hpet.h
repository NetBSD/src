#define	NHPET	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NHPET
 .global _KERNEL_OPT_NHPET
 .equiv _KERNEL_OPT_NHPET,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NHPET\n .global _KERNEL_OPT_NHPET\n .equiv _KERNEL_OPT_NHPET,0x0\n .endif");
#endif
