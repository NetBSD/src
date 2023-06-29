#define	BUFQ_FCFS	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BUFQ_FCFS
 .global _KERNEL_OPT_BUFQ_FCFS
 .equiv _KERNEL_OPT_BUFQ_FCFS,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BUFQ_FCFS\n .global _KERNEL_OPT_BUFQ_FCFS\n .equiv _KERNEL_OPT_BUFQ_FCFS,0x1\n .endif");
#endif
