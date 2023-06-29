#define	NWSFONT_GLUE	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWSFONT_GLUE
 .global _KERNEL_OPT_NWSFONT_GLUE
 .equiv _KERNEL_OPT_NWSFONT_GLUE,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWSFONT_GLUE\n .global _KERNEL_OPT_NWSFONT_GLUE\n .equiv _KERNEL_OPT_NWSFONT_GLUE,0x0\n .endif");
#endif
#define	NRASOPS_ROTATION	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NRASOPS_ROTATION
 .global _KERNEL_OPT_NRASOPS_ROTATION
 .equiv _KERNEL_OPT_NRASOPS_ROTATION,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NRASOPS_ROTATION\n .global _KERNEL_OPT_NRASOPS_ROTATION\n .equiv _KERNEL_OPT_NRASOPS_ROTATION,0x0\n .endif");
#endif
#define	NRASTERCONSOLE	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NRASTERCONSOLE
 .global _KERNEL_OPT_NRASTERCONSOLE
 .equiv _KERNEL_OPT_NRASTERCONSOLE,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NRASTERCONSOLE\n .global _KERNEL_OPT_NRASTERCONSOLE\n .equiv _KERNEL_OPT_NRASTERCONSOLE,0x0\n .endif");
#endif
#define	NWSDISPLAY	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWSDISPLAY
 .global _KERNEL_OPT_NWSDISPLAY
 .equiv _KERNEL_OPT_NWSDISPLAY,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWSDISPLAY\n .global _KERNEL_OPT_NWSDISPLAY\n .equiv _KERNEL_OPT_NWSDISPLAY,0x1\n .endif");
#endif
#define	NWSFONT	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWSFONT
 .global _KERNEL_OPT_NWSFONT
 .equiv _KERNEL_OPT_NWSFONT,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWSFONT\n .global _KERNEL_OPT_NWSFONT\n .equiv _KERNEL_OPT_NWSFONT,0x0\n .endif");
#endif
#define	NVCONS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NVCONS
 .global _KERNEL_OPT_NVCONS
 .equiv _KERNEL_OPT_NVCONS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NVCONS\n .global _KERNEL_OPT_NVCONS\n .equiv _KERNEL_OPT_NVCONS,0x0\n .endif");
#endif
