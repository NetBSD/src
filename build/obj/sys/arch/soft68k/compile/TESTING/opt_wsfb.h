/* option `WSFB_FAKE_VGA_FB' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_WSFB_FAKE_VGA_FB
 .global _KERNEL_OPT_WSFB_FAKE_VGA_FB
 .equiv _KERNEL_OPT_WSFB_FAKE_VGA_FB,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_WSFB_FAKE_VGA_FB\n .global _KERNEL_OPT_WSFB_FAKE_VGA_FB\n .equiv _KERNEL_OPT_WSFB_FAKE_VGA_FB,0x6e074def\n .endif");
#endif
/* option `WSFB_ALLOW_OTHERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_WSFB_ALLOW_OTHERS
 .global _KERNEL_OPT_WSFB_ALLOW_OTHERS
 .equiv _KERNEL_OPT_WSFB_ALLOW_OTHERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_WSFB_ALLOW_OTHERS\n .global _KERNEL_OPT_WSFB_ALLOW_OTHERS\n .equiv _KERNEL_OPT_WSFB_ALLOW_OTHERS,0x6e074def\n .endif");
#endif
