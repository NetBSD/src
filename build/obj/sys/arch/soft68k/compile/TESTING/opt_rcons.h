/* option `RASTERCONSOLE_BGCOL' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RASTERCONSOLE_BGCOL
 .global _KERNEL_OPT_RASTERCONSOLE_BGCOL
 .equiv _KERNEL_OPT_RASTERCONSOLE_BGCOL,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RASTERCONSOLE_BGCOL\n .global _KERNEL_OPT_RASTERCONSOLE_BGCOL\n .equiv _KERNEL_OPT_RASTERCONSOLE_BGCOL,0x6e074def\n .endif");
#endif
/* option `RASTERCONSOLE_FGCOL' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RASTERCONSOLE_FGCOL
 .global _KERNEL_OPT_RASTERCONSOLE_FGCOL
 .equiv _KERNEL_OPT_RASTERCONSOLE_FGCOL,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RASTERCONSOLE_FGCOL\n .global _KERNEL_OPT_RASTERCONSOLE_FGCOL\n .equiv _KERNEL_OPT_RASTERCONSOLE_FGCOL,0x6e074def\n .endif");
#endif
/* option `RCONS_16BPP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RCONS_16BPP
 .global _KERNEL_OPT_RCONS_16BPP
 .equiv _KERNEL_OPT_RCONS_16BPP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RCONS_16BPP\n .global _KERNEL_OPT_RCONS_16BPP\n .equiv _KERNEL_OPT_RCONS_16BPP,0x6e074def\n .endif");
#endif
/* option `RCONS_4BPP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RCONS_4BPP
 .global _KERNEL_OPT_RCONS_4BPP
 .equiv _KERNEL_OPT_RCONS_4BPP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RCONS_4BPP\n .global _KERNEL_OPT_RCONS_4BPP\n .equiv _KERNEL_OPT_RCONS_4BPP,0x6e074def\n .endif");
#endif
/* option `RCONS_2BPP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RCONS_2BPP
 .global _KERNEL_OPT_RCONS_2BPP
 .equiv _KERNEL_OPT_RCONS_2BPP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RCONS_2BPP\n .global _KERNEL_OPT_RCONS_2BPP\n .equiv _KERNEL_OPT_RCONS_2BPP,0x6e074def\n .endif");
#endif
