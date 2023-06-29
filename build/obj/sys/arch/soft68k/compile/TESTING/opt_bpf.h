/* option `BPF_BUFSIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BPF_BUFSIZE
 .global _KERNEL_OPT_BPF_BUFSIZE
 .equiv _KERNEL_OPT_BPF_BUFSIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BPF_BUFSIZE\n .global _KERNEL_OPT_BPF_BUFSIZE\n .equiv _KERNEL_OPT_BPF_BUFSIZE,0x6e074def\n .endif");
#endif
/* option `BPFJIT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BPFJIT
 .global _KERNEL_OPT_BPFJIT
 .equiv _KERNEL_OPT_BPFJIT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BPFJIT\n .global _KERNEL_OPT_BPFJIT\n .equiv _KERNEL_OPT_BPFJIT,0x6e074def\n .endif");
#endif
