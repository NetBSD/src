/* option `DRM_LEGACY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DRM_LEGACY
 .global _KERNEL_OPT_DRM_LEGACY
 .equiv _KERNEL_OPT_DRM_LEGACY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DRM_LEGACY\n .global _KERNEL_OPT_DRM_LEGACY\n .equiv _KERNEL_OPT_DRM_LEGACY,0x6e074def\n .endif");
#endif
