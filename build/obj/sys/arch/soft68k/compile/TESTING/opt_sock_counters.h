/* option `SOSEND_COUNTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SOSEND_COUNTERS
 .global _KERNEL_OPT_SOSEND_COUNTERS
 .equiv _KERNEL_OPT_SOSEND_COUNTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SOSEND_COUNTERS\n .global _KERNEL_OPT_SOSEND_COUNTERS\n .equiv _KERNEL_OPT_SOSEND_COUNTERS,0x6e074def\n .endif");
#endif
