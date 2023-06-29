/* option `PANICBUTTON' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PANICBUTTON
 .global _KERNEL_OPT_PANICBUTTON
 .equiv _KERNEL_OPT_PANICBUTTON,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PANICBUTTON\n .global _KERNEL_OPT_PANICBUTTON\n .equiv _KERNEL_OPT_PANICBUTTON,0x6e074def\n .endif");
#endif
