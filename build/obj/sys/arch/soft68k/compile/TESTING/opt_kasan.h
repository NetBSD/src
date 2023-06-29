/* option `KASAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KASAN
 .global _KERNEL_OPT_KASAN
 .equiv _KERNEL_OPT_KASAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KASAN\n .global _KERNEL_OPT_KASAN\n .equiv _KERNEL_OPT_KASAN,0x6e074def\n .endif");
#endif
/* option `KASAN_PANIC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KASAN_PANIC
 .global _KERNEL_OPT_KASAN_PANIC
 .equiv _KERNEL_OPT_KASAN_PANIC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KASAN_PANIC\n .global _KERNEL_OPT_KASAN_PANIC\n .equiv _KERNEL_OPT_KASAN_PANIC,0x6e074def\n .endif");
#endif
