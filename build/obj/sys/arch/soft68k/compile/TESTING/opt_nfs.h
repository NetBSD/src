/* option `NFS_V2_ONLY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_V2_ONLY
 .global _KERNEL_OPT_NFS_V2_ONLY
 .equiv _KERNEL_OPT_NFS_V2_ONLY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_V2_ONLY\n .global _KERNEL_OPT_NFS_V2_ONLY\n .equiv _KERNEL_OPT_NFS_V2_ONLY,0x6e074def\n .endif");
#endif
