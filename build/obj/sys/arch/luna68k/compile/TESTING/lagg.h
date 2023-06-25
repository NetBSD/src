#define	NLAGG	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NLAGG
 .global _KERNEL_OPT_NLAGG
 .equiv _KERNEL_OPT_NLAGG,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NLAGG\n .global _KERNEL_OPT_NLAGG\n .equiv _KERNEL_OPT_NLAGG,0x0\n .endif");
#endif
#define	NETHER	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NETHER
 .global _KERNEL_OPT_NETHER
 .equiv _KERNEL_OPT_NETHER,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NETHER\n .global _KERNEL_OPT_NETHER\n .equiv _KERNEL_OPT_NETHER,0x1\n .endif");
#endif
