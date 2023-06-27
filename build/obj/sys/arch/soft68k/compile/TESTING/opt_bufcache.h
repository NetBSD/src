/* option `BUFPAGES' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BUFPAGES
 .global _KERNEL_OPT_BUFPAGES
 .equiv _KERNEL_OPT_BUFPAGES,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BUFPAGES\n .global _KERNEL_OPT_BUFPAGES\n .equiv _KERNEL_OPT_BUFPAGES,0x6e074def\n .endif");
#endif
/* option `BUFCACHE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BUFCACHE
 .global _KERNEL_OPT_BUFCACHE
 .equiv _KERNEL_OPT_BUFCACHE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BUFCACHE\n .global _KERNEL_OPT_BUFCACHE\n .equiv _KERNEL_OPT_BUFCACHE,0x6e074def\n .endif");
#endif
