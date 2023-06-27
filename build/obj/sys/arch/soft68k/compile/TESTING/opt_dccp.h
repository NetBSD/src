/* option `ACKDEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ACKDEBUG
 .global _KERNEL_OPT_ACKDEBUG
 .equiv _KERNEL_OPT_ACKDEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ACKDEBUG\n .global _KERNEL_OPT_ACKDEBUG\n .equiv _KERNEL_OPT_ACKDEBUG,0x6e074def\n .endif");
#endif
/* option `DCCP_DEBUG_ON' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DCCP_DEBUG_ON
 .global _KERNEL_OPT_DCCP_DEBUG_ON
 .equiv _KERNEL_OPT_DCCP_DEBUG_ON,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DCCP_DEBUG_ON\n .global _KERNEL_OPT_DCCP_DEBUG_ON\n .equiv _KERNEL_OPT_DCCP_DEBUG_ON,0x6e074def\n .endif");
#endif
/* option `DCCPBHASHSIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DCCPBHASHSIZE
 .global _KERNEL_OPT_DCCPBHASHSIZE
 .equiv _KERNEL_OPT_DCCPBHASHSIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DCCPBHASHSIZE\n .global _KERNEL_OPT_DCCPBHASHSIZE\n .equiv _KERNEL_OPT_DCCPBHASHSIZE,0x6e074def\n .endif");
#endif
/* option `DCCPSTATES' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DCCPSTATES
 .global _KERNEL_OPT_DCCPSTATES
 .equiv _KERNEL_OPT_DCCPSTATES,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DCCPSTATES\n .global _KERNEL_OPT_DCCPSTATES\n .equiv _KERNEL_OPT_DCCPSTATES,0x6e074def\n .endif");
#endif
/* option `DCCP_TFRC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DCCP_TFRC
 .global _KERNEL_OPT_DCCP_TFRC
 .equiv _KERNEL_OPT_DCCP_TFRC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DCCP_TFRC\n .global _KERNEL_OPT_DCCP_TFRC\n .equiv _KERNEL_OPT_DCCP_TFRC,0x6e074def\n .endif");
#endif
/* option `DCCP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DCCP
 .global _KERNEL_OPT_DCCP
 .equiv _KERNEL_OPT_DCCP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DCCP\n .global _KERNEL_OPT_DCCP\n .equiv _KERNEL_OPT_DCCP,0x6e074def\n .endif");
#endif
