/* option `DEBUG_EXEC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DEBUG_EXEC
 .global _KERNEL_OPT_DEBUG_EXEC
 .equiv _KERNEL_OPT_DEBUG_EXEC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DEBUG_EXEC\n .global _KERNEL_OPT_DEBUG_EXEC\n .equiv _KERNEL_OPT_DEBUG_EXEC,0x6e074def\n .endif");
#endif
