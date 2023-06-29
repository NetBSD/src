/* option `COMPAT_OSSAUDIO' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_OSSAUDIO
 .global _KERNEL_OPT_COMPAT_OSSAUDIO
 .equiv _KERNEL_OPT_COMPAT_OSSAUDIO,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_OSSAUDIO\n .global _KERNEL_OPT_COMPAT_OSSAUDIO\n .equiv _KERNEL_OPT_COMPAT_OSSAUDIO,0x6e074def\n .endif");
#endif
