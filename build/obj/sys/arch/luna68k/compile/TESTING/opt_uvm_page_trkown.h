/* option `UVM_PAGE_TRKOWN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVM_PAGE_TRKOWN
 .global _KERNEL_OPT_UVM_PAGE_TRKOWN
 .equiv _KERNEL_OPT_UVM_PAGE_TRKOWN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVM_PAGE_TRKOWN\n .global _KERNEL_OPT_UVM_PAGE_TRKOWN\n .equiv _KERNEL_OPT_UVM_PAGE_TRKOWN,0x6e074def\n .endif");
#endif
