#define	NACT8846PM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NACT8846PM
 .global _KERNEL_OPT_NACT8846PM
 .equiv _KERNEL_OPT_NACT8846PM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NACT8846PM\n .global _KERNEL_OPT_NACT8846PM\n .equiv _KERNEL_OPT_NACT8846PM,0x0\n .endif");
#endif
