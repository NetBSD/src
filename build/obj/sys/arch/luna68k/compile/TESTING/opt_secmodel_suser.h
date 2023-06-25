#define	secmodel_suser	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_secmodel_suser
 .global _KERNEL_OPT_secmodel_suser
 .equiv _KERNEL_OPT_secmodel_suser,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_secmodel_suser\n .global _KERNEL_OPT_secmodel_suser\n .equiv _KERNEL_OPT_secmodel_suser,0x1\n .endif");
#endif
