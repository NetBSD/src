#define	NSYSMON_WDOG	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSYSMON_WDOG
 .global _KERNEL_OPT_NSYSMON_WDOG
 .equiv _KERNEL_OPT_NSYSMON_WDOG,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSYSMON_WDOG\n .global _KERNEL_OPT_NSYSMON_WDOG\n .equiv _KERNEL_OPT_NSYSMON_WDOG,0x1\n .endif");
#endif
