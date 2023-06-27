/* option `DDB_VERBOSE_HELP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DDB_VERBOSE_HELP
 .global _KERNEL_OPT_DDB_VERBOSE_HELP
 .equiv _KERNEL_OPT_DDB_VERBOSE_HELP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DDB_VERBOSE_HELP\n .global _KERNEL_OPT_DDB_VERBOSE_HELP\n .equiv _KERNEL_OPT_DDB_VERBOSE_HELP,0x6e074def\n .endif");
#endif
#define	DDB	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DDB
 .global _KERNEL_OPT_DDB
 .equiv _KERNEL_OPT_DDB,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DDB\n .global _KERNEL_OPT_DDB\n .equiv _KERNEL_OPT_DDB,0x1\n .endif");
#endif
