#define	NARCNET	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NARCNET
 .global _KERNEL_OPT_NARCNET
 .equiv _KERNEL_OPT_NARCNET,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NARCNET\n .global _KERNEL_OPT_NARCNET\n .equiv _KERNEL_OPT_NARCNET,0x0\n .endif");
#endif
