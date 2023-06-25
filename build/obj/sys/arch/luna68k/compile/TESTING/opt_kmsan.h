/* option `KMSAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KMSAN
 .global _KERNEL_OPT_KMSAN
 .equiv _KERNEL_OPT_KMSAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KMSAN\n .global _KERNEL_OPT_KMSAN\n .equiv _KERNEL_OPT_KMSAN,0x6e074def\n .endif");
#endif
/* option `KMSAN_PANIC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KMSAN_PANIC
 .global _KERNEL_OPT_KMSAN_PANIC
 .equiv _KERNEL_OPT_KMSAN_PANIC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KMSAN_PANIC\n .global _KERNEL_OPT_KMSAN_PANIC\n .equiv _KERNEL_OPT_KMSAN_PANIC,0x6e074def\n .endif");
#endif
