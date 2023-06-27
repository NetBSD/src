#define	NARP	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NARP
 .global _KERNEL_OPT_NARP
 .equiv _KERNEL_OPT_NARP,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NARP\n .global _KERNEL_OPT_NARP\n .equiv _KERNEL_OPT_NARP,0x1\n .endif");
#endif
#define	NNETATALK	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NNETATALK
 .global _KERNEL_OPT_NNETATALK
 .equiv _KERNEL_OPT_NNETATALK,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NNETATALK\n .global _KERNEL_OPT_NNETATALK\n .equiv _KERNEL_OPT_NNETATALK,0x0\n .endif");
#endif
