#define	NCLMPCC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NCLMPCC
 .global _KERNEL_OPT_NCLMPCC
 .equiv _KERNEL_OPT_NCLMPCC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NCLMPCC\n .global _KERNEL_OPT_NCLMPCC\n .equiv _KERNEL_OPT_NCLMPCC,0x0\n .endif");
#endif
