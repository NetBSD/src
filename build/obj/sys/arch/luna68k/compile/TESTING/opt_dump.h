/* option `DUMP_ON_PANIC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DUMP_ON_PANIC
 .global _KERNEL_OPT_DUMP_ON_PANIC
 .equiv _KERNEL_OPT_DUMP_ON_PANIC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DUMP_ON_PANIC\n .global _KERNEL_OPT_DUMP_ON_PANIC\n .equiv _KERNEL_OPT_DUMP_ON_PANIC,0x6e074def\n .endif");
#endif
