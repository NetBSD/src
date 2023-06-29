/* option `UVMMAP_COUNTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVMMAP_COUNTERS
 .global _KERNEL_OPT_UVMMAP_COUNTERS
 .equiv _KERNEL_OPT_UVMMAP_COUNTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVMMAP_COUNTERS\n .global _KERNEL_OPT_UVMMAP_COUNTERS\n .equiv _KERNEL_OPT_UVMMAP_COUNTERS,0x6e074def\n .endif");
#endif
/* option `UVM_RESERVED_PAGES_PER_CPU' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVM_RESERVED_PAGES_PER_CPU
 .global _KERNEL_OPT_UVM_RESERVED_PAGES_PER_CPU
 .equiv _KERNEL_OPT_UVM_RESERVED_PAGES_PER_CPU,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVM_RESERVED_PAGES_PER_CPU\n .global _KERNEL_OPT_UVM_RESERVED_PAGES_PER_CPU\n .equiv _KERNEL_OPT_UVM_RESERVED_PAGES_PER_CPU,0x6e074def\n .endif");
#endif
/* option `UVM' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVM
 .global _KERNEL_OPT_UVM
 .equiv _KERNEL_OPT_UVM,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVM\n .global _KERNEL_OPT_UVM\n .equiv _KERNEL_OPT_UVM,0x6e074def\n .endif");
#endif
