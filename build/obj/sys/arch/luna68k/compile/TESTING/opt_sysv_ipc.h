/* option `SYSV_IPC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SYSV_IPC
 .global _KERNEL_OPT_SYSV_IPC
 .equiv _KERNEL_OPT_SYSV_IPC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SYSV_IPC\n .global _KERNEL_OPT_SYSV_IPC\n .equiv _KERNEL_OPT_SYSV_IPC,0x6e074def\n .endif");
#endif
