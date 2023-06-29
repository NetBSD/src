#define	NNULLCONS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NNULLCONS
 .global _KERNEL_OPT_NNULLCONS
 .equiv _KERNEL_OPT_NNULLCONS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NNULLCONS\n .global _KERNEL_OPT_NNULLCONS\n .equiv _KERNEL_OPT_NNULLCONS,0x0\n .endif");
#endif
