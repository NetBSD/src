#define	NOFDISK	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NOFDISK
 .global _KERNEL_OPT_NOFDISK
 .equiv _KERNEL_OPT_NOFDISK,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NOFDISK\n .global _KERNEL_OPT_NOFDISK\n .equiv _KERNEL_OPT_NOFDISK,0x0\n .endif");
#endif
