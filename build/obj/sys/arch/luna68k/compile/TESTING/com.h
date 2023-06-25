#define	NCOM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NCOM
 .global _KERNEL_OPT_NCOM
 .equiv _KERNEL_OPT_NCOM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NCOM\n .global _KERNEL_OPT_NCOM\n .equiv _KERNEL_OPT_NCOM,0x0\n .endif");
#endif
