#define	NPPPOE	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPPPOE
 .global _KERNEL_OPT_NPPPOE
 .equiv _KERNEL_OPT_NPPPOE,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPPPOE\n .global _KERNEL_OPT_NPPPOE\n .equiv _KERNEL_OPT_NPPPOE,0x0\n .endif");
#endif
