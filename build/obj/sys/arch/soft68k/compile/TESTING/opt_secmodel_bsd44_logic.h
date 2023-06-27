#define	secmodel_bsd44_logic	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_secmodel_bsd44_logic
 .global _KERNEL_OPT_secmodel_bsd44_logic
 .equiv _KERNEL_OPT_secmodel_bsd44_logic,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_secmodel_bsd44_logic\n .global _KERNEL_OPT_secmodel_bsd44_logic\n .equiv _KERNEL_OPT_secmodel_bsd44_logic,0x1\n .endif");
#endif
