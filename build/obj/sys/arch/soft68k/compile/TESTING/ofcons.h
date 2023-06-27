#define	NOFCONS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NOFCONS
 .global _KERNEL_OPT_NOFCONS
 .equiv _KERNEL_OPT_NOFCONS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NOFCONS\n .global _KERNEL_OPT_NOFCONS\n .equiv _KERNEL_OPT_NOFCONS,0x0\n .endif");
#endif
