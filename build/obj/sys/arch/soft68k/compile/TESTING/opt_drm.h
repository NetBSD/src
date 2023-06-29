/* option `DRM_NO_MTRR' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DRM_NO_MTRR
 .global _KERNEL_OPT_DRM_NO_MTRR
 .equiv _KERNEL_OPT_DRM_NO_MTRR,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DRM_NO_MTRR\n .global _KERNEL_OPT_DRM_NO_MTRR\n .equiv _KERNEL_OPT_DRM_NO_MTRR,0x6e074def\n .endif");
#endif
/* option `DRM_NO_AGP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DRM_NO_AGP
 .global _KERNEL_OPT_DRM_NO_AGP
 .equiv _KERNEL_OPT_DRM_NO_AGP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DRM_NO_AGP\n .global _KERNEL_OPT_DRM_NO_AGP\n .equiv _KERNEL_OPT_DRM_NO_AGP,0x6e074def\n .endif");
#endif
/* option `DRM_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DRM_DEBUG
 .global _KERNEL_OPT_DRM_DEBUG
 .equiv _KERNEL_OPT_DRM_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DRM_DEBUG\n .global _KERNEL_OPT_DRM_DEBUG\n .equiv _KERNEL_OPT_DRM_DEBUG,0x6e074def\n .endif");
#endif
