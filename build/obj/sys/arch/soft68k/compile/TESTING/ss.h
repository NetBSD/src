#define	NSS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSS
 .global _KERNEL_OPT_NSS
 .equiv _KERNEL_OPT_NSS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSS\n .global _KERNEL_OPT_NSS\n .equiv _KERNEL_OPT_NSS,0x0\n .endif");
#endif
