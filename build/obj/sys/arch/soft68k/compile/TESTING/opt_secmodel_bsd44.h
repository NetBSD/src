#define	secmodel_bsd44	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_secmodel_bsd44
 .global _KERNEL_OPT_secmodel_bsd44
 .equiv _KERNEL_OPT_secmodel_bsd44,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_secmodel_bsd44\n .global _KERNEL_OPT_secmodel_bsd44\n .equiv _KERNEL_OPT_secmodel_bsd44,0x1\n .endif");
#endif
