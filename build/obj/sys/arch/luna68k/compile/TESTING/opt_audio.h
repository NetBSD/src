/* option `AUDIO_BLK_MS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_AUDIO_BLK_MS
 .global _KERNEL_OPT_AUDIO_BLK_MS
 .equiv _KERNEL_OPT_AUDIO_BLK_MS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_AUDIO_BLK_MS\n .global _KERNEL_OPT_AUDIO_BLK_MS\n .equiv _KERNEL_OPT_AUDIO_BLK_MS,0x6e074def\n .endif");
#endif
