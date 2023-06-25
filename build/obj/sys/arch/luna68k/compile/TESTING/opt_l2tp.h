/* option `L2TP_ID_HASH_SIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_L2TP_ID_HASH_SIZE
 .global _KERNEL_OPT_L2TP_ID_HASH_SIZE
 .equiv _KERNEL_OPT_L2TP_ID_HASH_SIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_L2TP_ID_HASH_SIZE\n .global _KERNEL_OPT_L2TP_ID_HASH_SIZE\n .equiv _KERNEL_OPT_L2TP_ID_HASH_SIZE,0x6e074def\n .endif");
#endif
