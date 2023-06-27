#define	NIRFRAME	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NIRFRAME
 .global _KERNEL_OPT_NIRFRAME
 .equiv _KERNEL_OPT_NIRFRAME,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NIRFRAME\n .global _KERNEL_OPT_NIRFRAME\n .equiv _KERNEL_OPT_NIRFRAME,0x0\n .endif");
#endif
#define	NIRFRAMEDRV	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NIRFRAMEDRV
 .global _KERNEL_OPT_NIRFRAMEDRV
 .equiv _KERNEL_OPT_NIRFRAMEDRV,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NIRFRAMEDRV\n .global _KERNEL_OPT_NIRFRAMEDRV\n .equiv _KERNEL_OPT_NIRFRAMEDRV,0x0\n .endif");
#endif
