#define	SCHED_4BSD	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCHED_4BSD
 .global _KERNEL_OPT_SCHED_4BSD
 .equiv _KERNEL_OPT_SCHED_4BSD,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCHED_4BSD\n .global _KERNEL_OPT_SCHED_4BSD\n .equiv _KERNEL_OPT_SCHED_4BSD,0x1\n .endif");
#endif
/* option `SCHED_M2' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCHED_M2
 .global _KERNEL_OPT_SCHED_M2
 .equiv _KERNEL_OPT_SCHED_M2,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCHED_M2\n .global _KERNEL_OPT_SCHED_M2\n .equiv _KERNEL_OPT_SCHED_M2,0x6e074def\n .endif");
#endif
