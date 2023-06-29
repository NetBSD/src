#define	NACPICA	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NACPICA
 .global _KERNEL_OPT_NACPICA
 .equiv _KERNEL_OPT_NACPICA,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NACPICA\n .global _KERNEL_OPT_NACPICA\n .equiv _KERNEL_OPT_NACPICA,0x0\n .endif");
#endif
