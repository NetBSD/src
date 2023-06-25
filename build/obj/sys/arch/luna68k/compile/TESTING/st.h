#define	NST	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NST
 .global _KERNEL_OPT_NST
 .equiv _KERNEL_OPT_NST,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NST\n .global _KERNEL_OPT_NST\n .equiv _KERNEL_OPT_NST,0x1\n .endif");
#endif
#define	NST_SCSIBUS	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NST_SCSIBUS
 .global _KERNEL_OPT_NST_SCSIBUS
 .equiv _KERNEL_OPT_NST_SCSIBUS,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NST_SCSIBUS\n .global _KERNEL_OPT_NST_SCSIBUS\n .equiv _KERNEL_OPT_NST_SCSIBUS,0x1\n .endif");
#endif
#define	NST_ATAPIBUS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NST_ATAPIBUS
 .global _KERNEL_OPT_NST_ATAPIBUS
 .equiv _KERNEL_OPT_NST_ATAPIBUS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NST_ATAPIBUS\n .global _KERNEL_OPT_NST_ATAPIBUS\n .equiv _KERNEL_OPT_NST_ATAPIBUS,0x0\n .endif");
#endif
