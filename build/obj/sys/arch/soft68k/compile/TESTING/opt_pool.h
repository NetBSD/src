/* option `POOL_QUARANTINE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_POOL_QUARANTINE
 .global _KERNEL_OPT_POOL_QUARANTINE
 .equiv _KERNEL_OPT_POOL_QUARANTINE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_POOL_QUARANTINE\n .global _KERNEL_OPT_POOL_QUARANTINE\n .equiv _KERNEL_OPT_POOL_QUARANTINE,0x6e074def\n .endif");
#endif
/* option `POOL_NOCACHE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_POOL_NOCACHE
 .global _KERNEL_OPT_POOL_NOCACHE
 .equiv _KERNEL_OPT_POOL_NOCACHE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_POOL_NOCACHE\n .global _KERNEL_OPT_POOL_NOCACHE\n .equiv _KERNEL_OPT_POOL_NOCACHE,0x6e074def\n .endif");
#endif
