/* option `PPPOE_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPPOE_DEBUG
 .global _KERNEL_OPT_PPPOE_DEBUG
 .equiv _KERNEL_OPT_PPPOE_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPPOE_DEBUG\n .global _KERNEL_OPT_PPPOE_DEBUG\n .equiv _KERNEL_OPT_PPPOE_DEBUG,0x6e074def\n .endif");
#endif
/* option `PPPOE_SERVER' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPPOE_SERVER
 .global _KERNEL_OPT_PPPOE_SERVER
 .equiv _KERNEL_OPT_PPPOE_SERVER,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPPOE_SERVER\n .global _KERNEL_OPT_PPPOE_SERVER\n .equiv _KERNEL_OPT_PPPOE_SERVER,0x6e074def\n .endif");
#endif
/* option `PPPOE_DEQUEUE_MAXLEN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPPOE_DEQUEUE_MAXLEN
 .global _KERNEL_OPT_PPPOE_DEQUEUE_MAXLEN
 .equiv _KERNEL_OPT_PPPOE_DEQUEUE_MAXLEN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPPOE_DEQUEUE_MAXLEN\n .global _KERNEL_OPT_PPPOE_DEQUEUE_MAXLEN\n .equiv _KERNEL_OPT_PPPOE_DEQUEUE_MAXLEN,0x6e074def\n .endif");
#endif
