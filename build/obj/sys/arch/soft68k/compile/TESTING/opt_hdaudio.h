/* option `HDAUDIO_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_HDAUDIO_DEBUG
 .global _KERNEL_OPT_HDAUDIO_DEBUG
 .equiv _KERNEL_OPT_HDAUDIO_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_HDAUDIO_DEBUG\n .global _KERNEL_OPT_HDAUDIO_DEBUG\n .equiv _KERNEL_OPT_HDAUDIO_DEBUG,0x6e074def\n .endif");
#endif
/* option `HDAFG_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_HDAFG_DEBUG
 .global _KERNEL_OPT_HDAFG_DEBUG
 .equiv _KERNEL_OPT_HDAFG_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_HDAFG_DEBUG\n .global _KERNEL_OPT_HDAFG_DEBUG\n .equiv _KERNEL_OPT_HDAFG_DEBUG,0x6e074def\n .endif");
#endif
/* option `HDAFG_HDMI_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_HDAFG_HDMI_DEBUG
 .global _KERNEL_OPT_HDAFG_HDMI_DEBUG
 .equiv _KERNEL_OPT_HDAFG_HDMI_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_HDAFG_HDMI_DEBUG\n .global _KERNEL_OPT_HDAFG_HDMI_DEBUG\n .equiv _KERNEL_OPT_HDAFG_HDMI_DEBUG,0x6e074def\n .endif");
#endif
