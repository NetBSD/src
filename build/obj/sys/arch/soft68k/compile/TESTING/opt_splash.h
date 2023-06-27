/* option `SPLASHSCREEN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SPLASHSCREEN
 .global _KERNEL_OPT_SPLASHSCREEN
 .equiv _KERNEL_OPT_SPLASHSCREEN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SPLASHSCREEN\n .global _KERNEL_OPT_SPLASHSCREEN\n .equiv _KERNEL_OPT_SPLASHSCREEN,0x6e074def\n .endif");
#endif
/* option `makeoptions_SPLASHSCREEN_IMAGE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_makeoptions_SPLASHSCREEN_IMAGE
 .global _KERNEL_OPT_makeoptions_SPLASHSCREEN_IMAGE
 .equiv _KERNEL_OPT_makeoptions_SPLASHSCREEN_IMAGE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_makeoptions_SPLASHSCREEN_IMAGE\n .global _KERNEL_OPT_makeoptions_SPLASHSCREEN_IMAGE\n .equiv _KERNEL_OPT_makeoptions_SPLASHSCREEN_IMAGE,0x6e074def\n .endif");
#endif
