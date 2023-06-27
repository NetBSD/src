#define	MAXUSERS	8
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MAXUSERS
 .global _KERNEL_OPT_MAXUSERS
 .equiv _KERNEL_OPT_MAXUSERS,0x8
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MAXUSERS\n .global _KERNEL_OPT_MAXUSERS\n .equiv _KERNEL_OPT_MAXUSERS,0x8\n .endif");
#endif
/* option `MSGBUFSIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MSGBUFSIZE
 .global _KERNEL_OPT_MSGBUFSIZE
 .equiv _KERNEL_OPT_MSGBUFSIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MSGBUFSIZE\n .global _KERNEL_OPT_MSGBUFSIZE\n .equiv _KERNEL_OPT_MSGBUFSIZE,0x6e074def\n .endif");
#endif
