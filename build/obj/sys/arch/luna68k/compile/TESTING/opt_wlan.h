/* option `WLAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_WLAN
 .global _KERNEL_OPT_WLAN
 .equiv _KERNEL_OPT_WLAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_WLAN\n .global _KERNEL_OPT_WLAN\n .equiv _KERNEL_OPT_WLAN,0x6e074def\n .endif");
#endif
