/* option `UFS_ACL' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UFS_ACL
 .global _KERNEL_OPT_UFS_ACL
 .equiv _KERNEL_OPT_UFS_ACL,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UFS_ACL\n .global _KERNEL_OPT_UFS_ACL\n .equiv _KERNEL_OPT_UFS_ACL,0x6e074def\n .endif");
#endif
/* option `UFS_EXTATTR' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UFS_EXTATTR
 .global _KERNEL_OPT_UFS_EXTATTR
 .equiv _KERNEL_OPT_UFS_EXTATTR,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UFS_EXTATTR\n .global _KERNEL_OPT_UFS_EXTATTR\n .equiv _KERNEL_OPT_UFS_EXTATTR,0x6e074def\n .endif");
#endif
/* option `UFS_DIRHASH' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UFS_DIRHASH
 .global _KERNEL_OPT_UFS_DIRHASH
 .equiv _KERNEL_OPT_UFS_DIRHASH,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UFS_DIRHASH\n .global _KERNEL_OPT_UFS_DIRHASH\n .equiv _KERNEL_OPT_UFS_DIRHASH,0x6e074def\n .endif");
#endif
/* option `APPLE_UFS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_APPLE_UFS
 .global _KERNEL_OPT_APPLE_UFS
 .equiv _KERNEL_OPT_APPLE_UFS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_APPLE_UFS\n .global _KERNEL_OPT_APPLE_UFS\n .equiv _KERNEL_OPT_APPLE_UFS,0x6e074def\n .endif");
#endif
/* option `FFS_NO_SNAPSHOT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FFS_NO_SNAPSHOT
 .global _KERNEL_OPT_FFS_NO_SNAPSHOT
 .equiv _KERNEL_OPT_FFS_NO_SNAPSHOT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FFS_NO_SNAPSHOT\n .global _KERNEL_OPT_FFS_NO_SNAPSHOT\n .equiv _KERNEL_OPT_FFS_NO_SNAPSHOT,0x6e074def\n .endif");
#endif
/* option `FFS_EI' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FFS_EI
 .global _KERNEL_OPT_FFS_EI
 .equiv _KERNEL_OPT_FFS_EI,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FFS_EI\n .global _KERNEL_OPT_FFS_EI\n .equiv _KERNEL_OPT_FFS_EI,0x6e074def\n .endif");
#endif
