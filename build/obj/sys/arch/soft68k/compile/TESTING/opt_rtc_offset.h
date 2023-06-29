/* option `RTC_OFFSET' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RTC_OFFSET
 .global _KERNEL_OPT_RTC_OFFSET
 .equiv _KERNEL_OPT_RTC_OFFSET,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RTC_OFFSET\n .global _KERNEL_OPT_RTC_OFFSET\n .equiv _KERNEL_OPT_RTC_OFFSET,0x6e074def\n .endif");
#endif
