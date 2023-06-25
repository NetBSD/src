#define	NOFRTC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NOFRTC
 .global _KERNEL_OPT_NOFRTC
 .equiv _KERNEL_OPT_NOFRTC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NOFRTC\n .global _KERNEL_OPT_NOFRTC\n .equiv _KERNEL_OPT_NOFRTC,0x0\n .endif");
#endif
