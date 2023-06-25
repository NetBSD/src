/* option `QUOTA2' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_QUOTA2
 .global _KERNEL_OPT_QUOTA2
 .equiv _KERNEL_OPT_QUOTA2,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_QUOTA2\n .global _KERNEL_OPT_QUOTA2\n .equiv _KERNEL_OPT_QUOTA2,0x6e074def\n .endif");
#endif
/* option `QUOTA' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_QUOTA
 .global _KERNEL_OPT_QUOTA
 .equiv _KERNEL_OPT_QUOTA,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_QUOTA\n .global _KERNEL_OPT_QUOTA\n .equiv _KERNEL_OPT_QUOTA,0x6e074def\n .endif");
#endif
