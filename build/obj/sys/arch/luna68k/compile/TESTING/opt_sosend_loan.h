/* option `SOSEND_NO_LOAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SOSEND_NO_LOAN
 .global _KERNEL_OPT_SOSEND_NO_LOAN
 .equiv _KERNEL_OPT_SOSEND_NO_LOAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SOSEND_NO_LOAN\n .global _KERNEL_OPT_SOSEND_NO_LOAN\n .equiv _KERNEL_OPT_SOSEND_NO_LOAN,0x6e074def\n .endif");
#endif
