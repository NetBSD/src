/* option `KDTRACE_HOOKS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KDTRACE_HOOKS
 .global _KERNEL_OPT_KDTRACE_HOOKS
 .equiv _KERNEL_OPT_KDTRACE_HOOKS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KDTRACE_HOOKS\n .global _KERNEL_OPT_KDTRACE_HOOKS\n .equiv _KERNEL_OPT_KDTRACE_HOOKS,0x6e074def\n .endif");
#endif
