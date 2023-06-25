/* option `CHIPSFB_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CHIPSFB_DEBUG
 .global _KERNEL_OPT_CHIPSFB_DEBUG
 .equiv _KERNEL_OPT_CHIPSFB_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CHIPSFB_DEBUG\n .global _KERNEL_OPT_CHIPSFB_DEBUG\n .equiv _KERNEL_OPT_CHIPSFB_DEBUG,0x6e074def\n .endif");
#endif
/* option `CHIPSFB_WAIT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CHIPSFB_WAIT
 .global _KERNEL_OPT_CHIPSFB_WAIT
 .equiv _KERNEL_OPT_CHIPSFB_WAIT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CHIPSFB_WAIT\n .global _KERNEL_OPT_CHIPSFB_WAIT\n .equiv _KERNEL_OPT_CHIPSFB_WAIT,0x6e074def\n .endif");
#endif
