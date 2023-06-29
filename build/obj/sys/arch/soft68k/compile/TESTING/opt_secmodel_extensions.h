#define	secmodel_extensions	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_secmodel_extensions
 .global _KERNEL_OPT_secmodel_extensions
 .equiv _KERNEL_OPT_secmodel_extensions,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_secmodel_extensions\n .global _KERNEL_OPT_secmodel_extensions\n .equiv _KERNEL_OPT_secmodel_extensions,0x1\n .endif");
#endif
