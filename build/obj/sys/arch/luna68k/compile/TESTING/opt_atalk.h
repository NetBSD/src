/* option `NETATALKDEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NETATALKDEBUG
 .global _KERNEL_OPT_NETATALKDEBUG
 .equiv _KERNEL_OPT_NETATALKDEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NETATALKDEBUG\n .global _KERNEL_OPT_NETATALKDEBUG\n .equiv _KERNEL_OPT_NETATALKDEBUG,0x6e074def\n .endif");
#endif
/* option `NETATALK' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NETATALK
 .global _KERNEL_OPT_NETATALK
 .equiv _KERNEL_OPT_NETATALK,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NETATALK\n .global _KERNEL_OPT_NETATALK\n .equiv _KERNEL_OPT_NETATALK,0x6e074def\n .endif");
#endif
