/* option `KCSAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KCSAN
 .global _KERNEL_OPT_KCSAN
 .equiv _KERNEL_OPT_KCSAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KCSAN\n .global _KERNEL_OPT_KCSAN\n .equiv _KERNEL_OPT_KCSAN,0x6e074def\n .endif");
#endif
/* option `KCSAN_PANIC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KCSAN_PANIC
 .global _KERNEL_OPT_KCSAN_PANIC
 .equiv _KERNEL_OPT_KCSAN_PANIC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KCSAN_PANIC\n .global _KERNEL_OPT_KCSAN_PANIC\n .equiv _KERNEL_OPT_KCSAN_PANIC,0x6e074def\n .endif");
#endif
