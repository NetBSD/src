/* option `RAID_AUTOCONFIG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RAID_AUTOCONFIG
 .global _KERNEL_OPT_RAID_AUTOCONFIG
 .equiv _KERNEL_OPT_RAID_AUTOCONFIG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RAID_AUTOCONFIG\n .global _KERNEL_OPT_RAID_AUTOCONFIG\n .equiv _KERNEL_OPT_RAID_AUTOCONFIG,0x6e074def\n .endif");
#endif
