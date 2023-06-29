/* option `IPX' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_IPX
 .global _KERNEL_OPT_IPX
 .equiv _KERNEL_OPT_IPX,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_IPX\n .global _KERNEL_OPT_IPX\n .equiv _KERNEL_OPT_IPX,0xdeadbeef\n .endif");
#endif
