/* option `UVM_HOTPLUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVM_HOTPLUG
 .global _KERNEL_OPT_UVM_HOTPLUG
 .equiv _KERNEL_OPT_UVM_HOTPLUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVM_HOTPLUG\n .global _KERNEL_OPT_UVM_HOTPLUG\n .equiv _KERNEL_OPT_UVM_HOTPLUG,0x6e074def\n .endif");
#endif
