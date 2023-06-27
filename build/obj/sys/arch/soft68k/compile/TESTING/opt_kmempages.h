/* option `NKMEMPAGES_MAX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NKMEMPAGES_MAX
 .global _KERNEL_OPT_NKMEMPAGES_MAX
 .equiv _KERNEL_OPT_NKMEMPAGES_MAX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NKMEMPAGES_MAX\n .global _KERNEL_OPT_NKMEMPAGES_MAX\n .equiv _KERNEL_OPT_NKMEMPAGES_MAX,0x6e074def\n .endif");
#endif
/* option `NKMEMPAGES_MIN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NKMEMPAGES_MIN
 .global _KERNEL_OPT_NKMEMPAGES_MIN
 .equiv _KERNEL_OPT_NKMEMPAGES_MIN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NKMEMPAGES_MIN\n .global _KERNEL_OPT_NKMEMPAGES_MIN\n .equiv _KERNEL_OPT_NKMEMPAGES_MIN,0x6e074def\n .endif");
#endif
/* option `NKMEMPAGES' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NKMEMPAGES
 .global _KERNEL_OPT_NKMEMPAGES
 .equiv _KERNEL_OPT_NKMEMPAGES,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NKMEMPAGES\n .global _KERNEL_OPT_NKMEMPAGES\n .equiv _KERNEL_OPT_NKMEMPAGES,0x6e074def\n .endif");
#endif
