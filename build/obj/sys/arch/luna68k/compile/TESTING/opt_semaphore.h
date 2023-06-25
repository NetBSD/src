/* option `SEMAPHORE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SEMAPHORE
 .global _KERNEL_OPT_SEMAPHORE
 .equiv _KERNEL_OPT_SEMAPHORE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SEMAPHORE\n .global _KERNEL_OPT_SEMAPHORE\n .equiv _KERNEL_OPT_SEMAPHORE,0x6e074def\n .endif");
#endif
