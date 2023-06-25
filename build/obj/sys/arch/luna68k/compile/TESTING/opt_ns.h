/* option `NSIP' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSIP
 .global _KERNEL_OPT_NSIP
 .equiv _KERNEL_OPT_NSIP,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSIP\n .global _KERNEL_OPT_NSIP\n .equiv _KERNEL_OPT_NSIP,0xdeadbeef\n .endif");
#endif
/* option `NS' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NS
 .global _KERNEL_OPT_NS
 .equiv _KERNEL_OPT_NS,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NS\n .global _KERNEL_OPT_NS\n .equiv _KERNEL_OPT_NS,0xdeadbeef\n .endif");
#endif
