/* option `HDLC' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_HDLC
 .global _KERNEL_OPT_HDLC
 .equiv _KERNEL_OPT_HDLC,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_HDLC\n .global _KERNEL_OPT_HDLC\n .equiv _KERNEL_OPT_HDLC,0xdeadbeef\n .endif");
#endif
