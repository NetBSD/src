/* option `VCONS_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_VCONS_DEBUG
 .global _KERNEL_OPT_VCONS_DEBUG
 .equiv _KERNEL_OPT_VCONS_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_VCONS_DEBUG\n .global _KERNEL_OPT_VCONS_DEBUG\n .equiv _KERNEL_OPT_VCONS_DEBUG,0x6e074def\n .endif");
#endif
/* option `VCONS_INTR_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_VCONS_INTR_DEBUG
 .global _KERNEL_OPT_VCONS_INTR_DEBUG
 .equiv _KERNEL_OPT_VCONS_INTR_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_VCONS_INTR_DEBUG\n .global _KERNEL_OPT_VCONS_INTR_DEBUG\n .equiv _KERNEL_OPT_VCONS_INTR_DEBUG,0x6e074def\n .endif");
#endif
/* option `VCONS_DRAW_INTR' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_VCONS_DRAW_INTR
 .global _KERNEL_OPT_VCONS_DRAW_INTR
 .equiv _KERNEL_OPT_VCONS_DRAW_INTR,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_VCONS_DRAW_INTR\n .global _KERNEL_OPT_VCONS_DRAW_INTR\n .equiv _KERNEL_OPT_VCONS_DRAW_INTR,0x6e074def\n .endif");
#endif
