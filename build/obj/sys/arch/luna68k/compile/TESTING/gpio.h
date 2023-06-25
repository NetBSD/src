#define	NGPIO	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NGPIO
 .global _KERNEL_OPT_NGPIO
 .equiv _KERNEL_OPT_NGPIO,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NGPIO\n .global _KERNEL_OPT_NGPIO\n .equiv _KERNEL_OPT_NGPIO,0x0\n .endif");
#endif
