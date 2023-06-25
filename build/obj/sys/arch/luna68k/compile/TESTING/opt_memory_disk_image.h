/* option `makeoptions_MEMORY_DISK_IMAGE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_makeoptions_MEMORY_DISK_IMAGE
 .global _KERNEL_OPT_makeoptions_MEMORY_DISK_IMAGE
 .equiv _KERNEL_OPT_makeoptions_MEMORY_DISK_IMAGE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_makeoptions_MEMORY_DISK_IMAGE\n .global _KERNEL_OPT_makeoptions_MEMORY_DISK_IMAGE\n .equiv _KERNEL_OPT_makeoptions_MEMORY_DISK_IMAGE,0x6e074def\n .endif");
#endif
