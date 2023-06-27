/* option `VNODE_LOCKDEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_VNODE_LOCKDEBUG
 .global _KERNEL_OPT_VNODE_LOCKDEBUG
 .equiv _KERNEL_OPT_VNODE_LOCKDEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_VNODE_LOCKDEBUG\n .global _KERNEL_OPT_VNODE_LOCKDEBUG\n .equiv _KERNEL_OPT_VNODE_LOCKDEBUG,0x6e074def\n .endif");
#endif
