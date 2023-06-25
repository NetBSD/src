/* option `FDSCRIPTS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FDSCRIPTS
 .global _KERNEL_OPT_FDSCRIPTS
 .equiv _KERNEL_OPT_FDSCRIPTS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FDSCRIPTS\n .global _KERNEL_OPT_FDSCRIPTS\n .equiv _KERNEL_OPT_FDSCRIPTS,0x6e074def\n .endif");
#endif
/* option `SETUIDSCRIPTS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SETUIDSCRIPTS
 .global _KERNEL_OPT_SETUIDSCRIPTS
 .equiv _KERNEL_OPT_SETUIDSCRIPTS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SETUIDSCRIPTS\n .global _KERNEL_OPT_SETUIDSCRIPTS\n .equiv _KERNEL_OPT_SETUIDSCRIPTS,0x6e074def\n .endif");
#endif
