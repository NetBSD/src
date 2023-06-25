#define	NWG	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWG
 .global _KERNEL_OPT_NWG
 .equiv _KERNEL_OPT_NWG,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWG\n .global _KERNEL_OPT_NWG\n .equiv _KERNEL_OPT_NWG,0x0\n .endif");
#endif
