/* option `PIPE_NODIRECT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PIPE_NODIRECT
 .global _KERNEL_OPT_PIPE_NODIRECT
 .equiv _KERNEL_OPT_PIPE_NODIRECT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PIPE_NODIRECT\n .global _KERNEL_OPT_PIPE_NODIRECT\n .equiv _KERNEL_OPT_PIPE_NODIRECT,0x6e074def\n .endif");
#endif
/* option `PIPE_SOCKETPAIR' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PIPE_SOCKETPAIR
 .global _KERNEL_OPT_PIPE_SOCKETPAIR
 .equiv _KERNEL_OPT_PIPE_SOCKETPAIR,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PIPE_SOCKETPAIR\n .global _KERNEL_OPT_PIPE_SOCKETPAIR\n .equiv _KERNEL_OPT_PIPE_SOCKETPAIR,0x6e074def\n .endif");
#endif
