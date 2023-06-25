/* option `HDAUDIO_ENABLE_HDMI' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_HDAUDIO_ENABLE_HDMI
 .global _KERNEL_OPT_HDAUDIO_ENABLE_HDMI
 .equiv _KERNEL_OPT_HDAUDIO_ENABLE_HDMI,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_HDAUDIO_ENABLE_HDMI\n .global _KERNEL_OPT_HDAUDIO_ENABLE_HDMI\n .equiv _KERNEL_OPT_HDAUDIO_ENABLE_HDMI,0xdeadbeef\n .endif");
#endif
