#define	NKSYMS	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NKSYMS
 .global _KERNEL_OPT_NKSYMS
 .equiv _KERNEL_OPT_NKSYMS,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NKSYMS\n .global _KERNEL_OPT_NKSYMS\n .equiv _KERNEL_OPT_NKSYMS,0x1\n .endif");
#endif
#define	NDDB	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NDDB
 .global _KERNEL_OPT_NDDB
 .equiv _KERNEL_OPT_NDDB,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NDDB\n .global _KERNEL_OPT_NDDB\n .equiv _KERNEL_OPT_NDDB,0x1\n .endif");
#endif
#define	NMODULAR	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NMODULAR
 .global _KERNEL_OPT_NMODULAR
 .equiv _KERNEL_OPT_NMODULAR,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NMODULAR\n .global _KERNEL_OPT_NMODULAR\n .equiv _KERNEL_OPT_NMODULAR,0x1\n .endif");
#endif
