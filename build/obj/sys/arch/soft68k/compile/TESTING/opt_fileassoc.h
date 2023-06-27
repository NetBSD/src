/* option `FILEASSOC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FILEASSOC
 .global _KERNEL_OPT_FILEASSOC
 .equiv _KERNEL_OPT_FILEASSOC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FILEASSOC\n .global _KERNEL_OPT_FILEASSOC\n .equiv _KERNEL_OPT_FILEASSOC,0x6e074def\n .endif");
#endif
