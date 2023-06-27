#define	NSPKR_AUDIO	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSPKR_AUDIO
 .global _KERNEL_OPT_NSPKR_AUDIO
 .equiv _KERNEL_OPT_NSPKR_AUDIO,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSPKR_AUDIO\n .global _KERNEL_OPT_NSPKR_AUDIO\n .equiv _KERNEL_OPT_NSPKR_AUDIO,0x0\n .endif");
#endif
