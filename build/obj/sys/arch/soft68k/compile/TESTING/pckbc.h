#define	NPCKBC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPCKBC
 .global _KERNEL_OPT_NPCKBC
 .equiv _KERNEL_OPT_NPCKBC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPCKBC\n .global _KERNEL_OPT_NPCKBC\n .equiv _KERNEL_OPT_NPCKBC,0x0\n .endif");
#endif
