#define	secmodel_securelevel	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_secmodel_securelevel
 .global _KERNEL_OPT_secmodel_securelevel
 .equiv _KERNEL_OPT_secmodel_securelevel,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_secmodel_securelevel\n .global _KERNEL_OPT_secmodel_securelevel\n .equiv _KERNEL_OPT_secmodel_securelevel,0x1\n .endif");
#endif
