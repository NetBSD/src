#define	NSEQUENCER	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSEQUENCER
 .global _KERNEL_OPT_NSEQUENCER
 .equiv _KERNEL_OPT_NSEQUENCER,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSEQUENCER\n .global _KERNEL_OPT_NSEQUENCER\n .equiv _KERNEL_OPT_NSEQUENCER,0x0\n .endif");
#endif
